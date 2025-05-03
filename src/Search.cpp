#include "Search.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <thread>
#include <tuple>
#include <vector>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "EGTB.h"
#include "Evaluate.h"
#include "GameState.h"
#include "History.h"
#include "Move.h"
#include "MoveGeneration.h"
#include "MoveList.h"
#include "Network.h"
#include "Score.h"
#include "SearchConstants.h"
#include "SearchData.h"
#include "SearchLimits.h"
#include "StagedMoveGenerator.h"
#include "StaticExchangeEvaluation.h"
#include "StaticVector.h"
#include "TTEntry.h"
#include "TimeManager.h"
#include "TranspositionTable.h"
#include "atomic.h"
#include "uci/uci.h"

enum class SearchType
{
    ROOT,
    PV,
    ZW,
};

void SearchPosition(GameState& position, SearchLocalState& local, SearchSharedState& shared);

Score AspirationWindowSearch(
    GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared, Score mid_score);

template <SearchType search_type>
Score NegaScout(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared,
    int depth, Score alpha, Score beta);

template <SearchType search_type>
Score Quiescence(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared,
    int depth, Score alpha, Score beta);

void SearchThread(GameState& position, SearchSharedState& shared)
{
#ifdef TUNE
    LMR_reduction = Initialise_LMR_reduction();
#endif

    shared.ResetNewSearch();

    // Limit the MultiPV setting to be at most the number of legal moves
    auto multi_pv = shared.get_multi_pv_setting();
    const auto checkers = Checkers(position.Board());
    const auto pinned = PinnedMask(position.Board(), position.Board().stm);
    BasicMoveList moves;
    LegalMoves(position.Board(), moves, pinned, checkers);
    auto legal_moves = std::count_if(moves.begin(), moves.end(),
        [&](const Move& move) { return MoveIsLegal(position.Board(), move, pinned, checkers); });
    multi_pv = std::min<int>(multi_pv, legal_moves);

    // Probe TB at root
    auto probe = Syzygy::probe_dtz_root(position.Board());
    BasicMoveList root_move_whitelist;
    if (probe.has_value())
    {
        // filter out the results which preserve the tbRank
        for (const auto& [move, _, tb_rank] : probe->root_moves)
        {
            if (tb_rank != probe->root_moves[0].tb_rank)
            {
                break;
            }

            root_move_whitelist.emplace_back(move);
        }

        multi_pv = std::min<int>(multi_pv, root_move_whitelist.size());
    }

    auto old_multi_pv = shared.get_multi_pv_setting();
    shared.set_multi_pv(multi_pv);
    shared.stop_searching = false;

    std::vector<std::thread> threads;

    // We directly run SearchPosition from the main thread, and launch the other threads for any additional threads.
    // This means in single threaded mode we can avoid launching any additional threads or having to copy position.
    for (int i = 1; i < shared.get_threads_setting(); i++)
    {
        auto& local = shared.get_local_state(i);
        local.root_move_whitelist = root_move_whitelist;
        local.net.Reset(position.Board(), local.search_stack.root()->acc);
        threads.emplace_back(
            std::thread([position, &local, &shared]() mutable { SearchPosition(position, local, shared); }));
    }

    {
        auto& local = shared.get_local_state(0);
        local.root_move_whitelist = root_move_whitelist;
        local.net.Reset(position.Board(), local.search_stack.root()->acc);
        SearchPosition(position, local, shared);
    }

    for (size_t i = 0; i < threads.size(); i++)
    {
        threads[i].join();
    }

    const auto& search_result = shared.get_best_search_result();
    shared.uci_handler.print_search_info(search_result, true);
    shared.uci_handler.print_bestmove(search_result.pv[0]);
    shared.set_multi_pv(old_multi_pv);
}

void SearchPosition(GameState& position, SearchLocalState& local, SearchSharedState& shared)
{
    auto* ss = local.search_stack.root();
    Score mid_score = 0;

    for (int depth = 1; depth < MAX_DEPTH; depth++)
    {
        if (shared.limits.depth && depth > shared.limits.depth)
        {
            return;
        }

        local.root_move_blacklist.clear();
        local.curr_depth = depth;

        for (int multi_pv = 1; multi_pv <= shared.get_multi_pv_setting(); multi_pv++)
        {
            local.curr_multi_pv = multi_pv;

            if (shared.limits.time && !shared.limits.time->ShouldContinueSearch())
            {
                shared.report_thread_wants_to_stop(local.thread_id);
            }

            auto score = AspirationWindowSearch(position, ss, local, shared, mid_score);

            if (local.aborting_search)
            {
                return;
            }

            if (multi_pv == 1)
            {
                mid_score = score;
            }

            local.root_move_blacklist.push_back(ss->pv[0]);
            shared.report_search_result(ss, local, score, SearchResultType::EXACT);
        }

        if (shared.limits.mate && Score::Limits::MATE - abs(mid_score.value()) <= shared.limits.mate.value() * 2)
        {
            return;
        }
    }
}

Score AspirationWindowSearch(
    GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared, Score mid_score)
{
    Score delta = 12;
    Score alpha = std::max<Score>(Score::Limits::MATED, mid_score - delta);
    Score beta = std::min<Score>(Score::Limits::MATE, mid_score + delta);

    while (true)
    {
        local.sel_septh = 0;
        auto score = NegaScout<SearchType::ROOT>(position, ss, local, shared, local.curr_depth, alpha, beta);

        if (local.aborting_search)
        {
            return SCORE_UNDEFINED;
        }

        if (alpha < score && score < beta)
        {
            return score;
        }

        if (score <= alpha)
        {
            score = alpha;
            shared.report_search_result(ss, local, score, SearchResultType::UPPER_BOUND);
            // Bring down beta on a fail low
            beta = (alpha + beta) / 2;
            alpha = std::max<Score>(Score::Limits::MATED, alpha - delta);
        }

        if (score >= beta)
        {
            score = beta;
            shared.report_search_result(ss, local, score, SearchResultType::LOWER_BOUND);
            beta = std::min<Score>(Score::Limits::MATE, beta + delta);
        }

        delta = delta + delta / 2;
    }
}

bool should_abort_search(SearchLocalState& local, const SearchSharedState& shared)
{
    // If we are currently in the process of aborting, do so as quickly as possible
    if (local.aborting_search)
    {
        return true;
    }

    // Avoid checking the limits too often
    if (local.limit_check_counter > 0)
    {
        local.limit_check_counter--;
        return false;
    }

    // No matter what, we always complete a depth 1 search.
    if (local.curr_depth <= 1)
    {
        return false;
    }

    // A signal that we recieved a UCI stop command, or the threads voted to stop searching
    if (shared.stop_searching)
    {
        local.aborting_search = true;
        return true;
    }

    if (shared.limits.time && shared.limits.time->ShouldAbortSearch())
    {
        local.aborting_search = true;
        return true;
    }

    uint64_t nodes = local.nodes;
    if (shared.limits.nodes && nodes >= shared.limits.nodes)
    {
        local.aborting_search = true;
        return true;
    }

    // Reset the limit_check_counter to 1024, or 1/2 the remaining node limit if smaller
    local.limit_check_counter
        = shared.limits.nodes ? std::clamp<int64_t>((*shared.limits.nodes - nodes) / 2, 0, 1024) : 1024;
    return false;
}

std::optional<Score> init_search_node(const GameState& position, const int distance_from_root, SearchStackState* ss,
    SearchLocalState& local, const SearchSharedState& shared)
{
    if (should_abort_search(local, shared))
    {
        return SCORE_UNDEFINED;
    }

    local.sel_septh = std::max(local.sel_septh, distance_from_root);
    local.nodes.fetch_add(1, std::memory_order_relaxed);

    if (distance_from_root >= MAX_DEPTH)
    {
        return 0;
    }

    ss->pv.clear();

    if (DeadPosition(position.Board()))
    {
        return 0;
    }

    // Draw randomness as in https://github.com/Luecx/Koivisto/commit/c8f01211c290a582b69e4299400b667a7731a9f7 with
    // permission from Koivisto authors. The condition > 100 is used because the 100th move could give checkmate.
    if (position.is_repitition(distance_from_root) || position.Board().fifty_move_count > 100)
    {
        return 8 - (local.nodes & 0b1111);
    }

    ss->multiple_extensions = (ss - 1)->multiple_extensions;

    if (!ss->nmp_verification_root)
    {
        ss->nmp_verification_depth = (ss - 1)->nmp_verification_depth;
    }

    return std::nullopt;
}

template <bool root_node, bool pv_node>
std::optional<Score> probe_egtb(const GameState& position, const int distance_from_root, SearchLocalState& local,
    SearchStackState* ss, Score& alpha, Score& beta, Score& min_score, Score& max_score, const int depth)
{
    auto probe = Syzygy::probe_wdl_search(position.Board(), distance_from_root);
    if (probe.has_value())
    {
        local.tb_hits.fetch_add(1, std::memory_order_relaxed);
        const auto tb_score = *probe;

        if constexpr (!root_node)
        {
            const auto bound = [tb_score]()
            {
                if (tb_score == Score::draw())
                {
                    return SearchResultType::EXACT;
                }
                else if (tb_score >= Score::tb_win_in(MAX_DEPTH))
                {
                    return SearchResultType::LOWER_BOUND;
                }
                else
                {
                    return SearchResultType::UPPER_BOUND;
                }
            }();

            if (bound == SearchResultType::EXACT || (bound == SearchResultType::LOWER_BOUND && tb_score >= beta)
                || (bound == SearchResultType::UPPER_BOUND && tb_score <= alpha))
            {
                tTable.AddEntry(Move::Uninitialized, position.Board().GetZobristKey(), tb_score, depth,
                    position.Board().half_turn_count, distance_from_root, bound, SCORE_UNDEFINED);
                return tb_score;
            }
        }

        // Why update score?
        // Because in a PV node we want the returned score to be accurate and reflect the TB score.
        // As such, we either set a cap for the score or raise the score to a minimum which can be further improved.
        // Remember, static evals will never reach these impossible tb-win/loss scores

        // Why don't we update score in non-PV nodes?
        // Because if we are in a non-pv node and didn't get a cutoff then we had one of two situations:
        // 1. We found a tb - win which is further from root than a tb - win from another line
        // 2. We found a tb - loss which is closer to root than a tb - loss from another line
        // Either way this node won't become part of the PV and so getting the correct score doesn't matter

        // Why do we update alpha?
        // Because we are spending effort exploring a subtree when we already know the result. All we actually
        // care about is whether there exists a forced mate or not from this node, and hence we raise alpha
        // to an impossible goal that prunes away all non-mate scores.

        // Why don't we raise alpha in non-pv nodes?
        // Because if we had a tb-win and the score < beta, then it must also be <= alpha remembering we are in a
        // zero width search and beta = alpha + 1.

        if constexpr (pv_node)
        {
            if (tb_score >= Score::tb_win_in(MAX_DEPTH))
            {
                min_score = tb_score;
                alpha = std::max(alpha, tb_score);

                if constexpr (root_node)
                {
                    // Because we raised alpha to a tb win, if we don't find a checkmate the root PV will end up empty.
                    // In this case, any move from the root move whitelist is acceptable
                    ss->pv.push_back(local.root_move_whitelist.front());
                }
            }
            else
            {
                max_score = tb_score;
                beta = std::min(beta, tb_score);
            }
        }
    }

    return std::nullopt;
}

std::tuple<TTEntry*, Score, int, SearchResultType, Move, Score> probe_tt(
    const GameState& position, const int distance_from_root)
{
    // copy the values out of the table that we want, to avoid race conditions
    auto* tt_entry
        = tTable.GetEntry(position.Board().GetZobristKey(), distance_from_root, position.Board().half_turn_count);
    const auto tt_score = tt_entry ? convert_from_tt_score(tt_entry->score, distance_from_root) : SCORE_UNDEFINED;
    const auto tt_depth = tt_entry ? tt_entry->depth : 0;
    const auto tt_cutoff = tt_entry ? tt_entry->meta.type : SearchResultType::EMPTY;
    const auto tt_move = tt_entry ? tt_entry->move : Move::Uninitialized;
    const auto tt_eval = tt_entry ? tt_entry->static_eval : SCORE_UNDEFINED;
    return { tt_entry, tt_score, tt_depth, tt_cutoff, tt_move, tt_eval };
}

std::optional<Score> tt_cutoff_node(const GameState& position, const Score tt_score, const SearchResultType tt_cutoff,
    const Score alpha, const Score beta)
{
    // Don't take scores from the TT if there's a two-fold repitition
    if (position.is_two_fold_repitition())
    {
        return std::nullopt;
    }

    if (tt_cutoff == SearchResultType::EXACT || (tt_cutoff == SearchResultType::LOWER_BOUND && tt_score >= beta)
        || (tt_cutoff == SearchResultType::UPPER_BOUND && tt_score <= alpha))
    {
        return tt_score;
    }

    return std::nullopt;
}

bool IsEndGame(const BoardState& board)
{
    return (
        board.GetPiecesColour(board.stm) == (board.GetPieceBB(KING, board.stm) | board.GetPieceBB(PAWN, board.stm)));
}

std::optional<Score> null_move_pruning(GameState& position, SearchStackState* ss, SearchLocalState& local,
    SearchSharedState& shared, const int distance_from_root, const int depth, const Score static_score,
    const Score beta)
{
    // avoid null move pruning in very late game positions due to zanauag issues.
    // Even with verification search e.g 8/6k1/8/8/8/8/1K6/Q7 w - - 0 1
    if (IsEndGame(position.Board()) || GetBitCount(position.Board().GetAllPieces()) < 5)
    {
        return std::nullopt;
    }

    const int reduction = 4 + depth / 6 + std::min(3, (static_score - beta).value() / 245);

    ss->move = Move::Uninitialized;
    ss->moved_piece = N_PIECES;
    ss->cont_hist_subtable = nullptr;
    position.ApplyNullMove();
    local.net.StoreLazyUpdates(position.PrevBoard(), position.Board(), (ss + 1)->acc, Move::Uninitialized);
    auto null_move_score
        = -NegaScout<SearchType::ZW>(position, ss + 1, local, shared, depth - reduction - 1, -beta, -beta + 1);
    position.RevertNullMove();

    if (null_move_score >= beta)
    {
        // As per: https://github.com/official-stockfish/Stockfish/pull/1338
        // Enhanced null move verification search. In order to detect zugzwang, traditional approaches will do a
        // reduced depth search on the current position disallowing null move pruning. This works, but does nothing
        // to avoid it's reaching a later similar zugzwang position during verification. To get around this, we
        // disable NMP for a portion of the remaining search tree. Disabling NMP is costly, and 3/4 was found to be
        // optimal

        if (depth < 10)
        {
            return beta;
        }

        ss->nmp_verification_depth = distance_from_root + 3 * (depth - reduction - 1) / 4;
        ss->nmp_verification_root = true;
        auto verification
            = NegaScout<SearchType::ZW>(position, ss, local, shared, depth - reduction - 1, beta - 1, beta);
        ss->nmp_verification_root = false;
        ss->nmp_verification_depth = 0;

        if (verification >= beta)
        {
            return beta;
        }
    }

    return std::nullopt;
}

template <bool pv_node>
std::optional<Score> singular_extensions(GameState& position, SearchStackState* ss, SearchLocalState& local,
    SearchSharedState& shared, int depth, const Score tt_score, const Move tt_move, const Score beta, int& extensions)
{
    Score sbeta = tt_score - 56 * depth / 64;
    int sdepth = depth / 2;

    ss->singular_exclusion = tt_move;

    auto se_score = NegaScout<SearchType::ZW>(position, ss, local, shared, sdepth, sbeta - 1, sbeta);

    ss->singular_exclusion = Move::Uninitialized;

    // Extending the SE idea, if the score is far below sbeta we extend by two. To avoid extending too much down
    // forced lines we limit the number of multiple_extensions down one line. We focus on non_pv nodes becuase
    // in particular we want to verify cut nodes which rest on a single good move and ensure we haven't
    // overlooked a potential non-pv line.
    if (!pv_node && se_score < sbeta - 11 && ss->multiple_extensions < 7)
    {
        extensions += 2;
        ss->multiple_extensions++;
    }
    else if (se_score < sbeta)
    {
        extensions += 1;
    }

    // Multi-Cut: In this case, we have proven that at least one other move appears to fail high, along with
    // the TT move having a LOWER_BOUND score of significantly above beta. In this case, we can assume the node
    // will fail high and we return a soft bound.
    else if (sbeta >= beta)
    {
        return sbeta;
    }

    // Negative extensions: if the TT move is not singular, but also doesn't appear good enough to multi-cut, we
    // might decide to reduce the TT move search. The TT move doesn't have LMR applied, to heuristically this
    // reduction can be thought of as evening out the search depth between the moves and not favouring the TT
    // move as heavily.
    else if (tt_score >= beta)
    {
        extensions += -1;
    }

    return std::nullopt;
}

template <bool pv_node>
int reduction(int depth, int seen_moves, int history)
{
    if (seen_moves == 1)
    {
        return 0;
    }

    int r = LMR_reduction[std::clamp(depth, 0, 63)][std::clamp(seen_moves, 0, 63)];

    if constexpr (pv_node)
        r--;

    r -= history / 7844;

    return std::max(0, r);
}

void UpdatePV(Move move, SearchStackState* ss)
{
    ss->pv.clear();
    ss->pv.emplace_back(move);
    ss->pv.insert(ss->pv.end(), (ss + 1)->pv.begin(), (ss + 1)->pv.end());
}

void AddKiller(Move move, std::array<Move, 2>& killers)
{
    if (move.IsCapture() || move.IsPromotion() || killers[0] == move)
        return;

    killers[1] = killers[0];
    killers[0] = move;
}

void AddHistory(const StagedMoveGenerator& gen, const Move& move, int depthRemaining)
{
    if (move.IsCapture() || move.IsPromotion())
    {
        gen.AdjustLoudHistory(move, depthRemaining * depthRemaining, -depthRemaining * depthRemaining);
    }
    else
    {
        gen.AdjustQuietHistory(move, depthRemaining * depthRemaining, -depthRemaining * depthRemaining);
    }
}

template <bool pv_node>
bool update_search_stats(SearchStackState* ss, StagedMoveGenerator& gen, const int depth, const Score search_score,
    const Move search_move, Score& best_score, Move& best_move, Score& alpha, const Score beta)
{
    if (search_score > best_score)
    {
        best_move = search_move;
        best_score = search_score;

        if (best_score > alpha)
        {
            alpha = best_score;

            if constexpr (pv_node)
            {
                UpdatePV(search_move, ss);
            }

            if (alpha >= beta)
            {
                AddKiller(search_move, ss->killers);
                AddHistory(gen, search_move, depth);
                return true;
            }
        }
    }

    return false;
}

template <bool pv_node>
Score search_move(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared,
    const int depth, const int extensions, const int reductions, const Score alpha, const Score beta,
    const int seen_moves)
{
    const int new_depth = depth + extensions - 1;
    Score search_score = 0;

    if (reductions > 0)
    {
        search_score
            = -NegaScout<SearchType::ZW>(position, ss + 1, local, shared, new_depth - reductions, -(alpha + 1), -alpha);

        if (search_score <= alpha)
        {
            return search_score;
        }
    }

    // If the reduced depth search was skipped or failed high, we do a full depth zero width search
    if (!pv_node || seen_moves > 1)
    {
        search_score = -NegaScout<SearchType::ZW>(position, ss + 1, local, shared, new_depth, -(alpha + 1), -alpha);
    }

    // If the ZW search was skipped or failed high, we do a full depth full width search
    if (pv_node && (seen_moves == 1 || (search_score > alpha && search_score < beta)))
    {
        search_score = -NegaScout<SearchType::PV>(position, ss + 1, local, shared, new_depth, -beta, -alpha);
    }

    return search_score;
}

Score TerminalScore(const BoardState& board, int distanceFromRoot)
{
    if (IsInCheck(board))
    {
        return Score::mated_in(distanceFromRoot);
    }
    else
    {
        return (Score::draw());
    }
}

// { raw, adjusted }
template <bool is_qsearch>
std::tuple<Score, Score> get_search_eval(const GameState& position, SearchStackState* ss, SearchLocalState& local,
    TTEntry* const tt_entry, const Score tt_eval, const Score tt_score, const SearchResultType tt_cutoff, int depth,
    int distance_from_root)
{
    Score raw_eval;
    Score adjusted_eval;

    auto scale_eval_50_move = [&position](Score eval)
    {
        // rescale and skew the raw eval based on the 50 move rule. We need to reclamp the score to ensure we don't
        // return false mate scores
        return eval.value() * (288 - (int)position.Board().fifty_move_count) / 256;
    };

    auto eval_corr_history = [&](Score eval) { return eval + local.pawn_corr_hist.get_correction_score(position); };

    if (tt_entry)
    {
        if (tt_eval != SCORE_UNDEFINED)
        {
            raw_eval = tt_eval;
        }
        else
        {
            tt_entry->static_eval = raw_eval = Evaluate(position.Board(), ss, local.net);
        }

        adjusted_eval = scale_eval_50_move(raw_eval);
        adjusted_eval = eval_corr_history(adjusted_eval);

        // Use the tt_score to improve the static eval if possible. Avoid returning unproved mate scores in q-search
        if (tt_score != SCORE_UNDEFINED && (!is_qsearch || std::abs(tt_score) < Score::tb_win_in(MAX_DEPTH))
            && (tt_cutoff == SearchResultType::EXACT
                || (tt_cutoff == SearchResultType::LOWER_BOUND && tt_score >= adjusted_eval)
                || (tt_cutoff == SearchResultType::UPPER_BOUND && tt_score <= adjusted_eval)))
        {
            adjusted_eval = tt_score;
        }
    }
    else
    {
        raw_eval = Evaluate(position.Board(), ss, local.net);
        adjusted_eval = scale_eval_50_move(raw_eval);
        adjusted_eval = eval_corr_history(adjusted_eval);
        tTable.AddEntry(Move::Uninitialized, position.Board().GetZobristKey(), SCORE_UNDEFINED, depth,
            position.Board().half_turn_count, distance_from_root, SearchResultType::EMPTY, raw_eval);
    }

    adjusted_eval = std::clamp<Score>(adjusted_eval, Score::Limits::EVAL_MIN, Score::Limits::EVAL_MAX);
    return { raw_eval, adjusted_eval };
}

void TestUpcomingCycleDetection(GameState& position, int distance_from_root, uint64_t pinned, uint64_t checkers)
{
    // upcoming_rep should return true iff there is a legal move that will lead to a repetition.
    // It's possible to have a zobrist hash collision, so this isn't a perfect test. But the likelihood of this
    // happening is very low.

    bool has_upcoming_rep = position.upcoming_rep(distance_from_root);
    bool is_draw = false;
    BasicMoveList moves;
    LegalMoves(position.Board(), moves, pinned, checkers);

    for (const auto& move : moves)
    {
        position.ApplyMove(move);
        if (position.is_repitition(distance_from_root + 1))
        {
            is_draw = true;
        }
        position.RevertMove();
    }

    if (has_upcoming_rep != is_draw)
    {
        std::cout << "Upcoming rep: " << has_upcoming_rep << " != " << is_draw << std::endl;
        std::cout << position.Board() << std::endl;
        position.upcoming_rep(distance_from_root);
    }
}

template <SearchType search_type>
Score NegaScout(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared,
    int depth, Score alpha, Score beta)
{
    assert((search_type != SearchType::ROOT) || (ss->distance_from_root == 0));
    assert((search_type != SearchType::ZW) || (beta == alpha + 1));
    constexpr bool pv_node = search_type != SearchType::ZW;
    constexpr bool root_node = search_type == SearchType::ROOT;
    const auto distance_from_root = ss->distance_from_root;
    const auto checkers = Checkers(position.Board());

    // Step 1: Drop into q-search
    if (depth <= 0 && !checkers)
    {
        constexpr SearchType qsearch_type = pv_node ? SearchType::PV : SearchType::ZW;
        return Quiescence<qsearch_type>(position, ss, local, shared, depth, alpha, beta);
    }

    // Step 2: Check for abort or draw and update stats in the local state and search stack
    if (auto value = init_search_node(position, distance_from_root, ss, local, shared))
    {
        return *value;
    }

    auto score = std::numeric_limits<Score>::min();
    auto max_score = std::numeric_limits<Score>::max();
    auto min_score = std::numeric_limits<Score>::min();

#ifdef TEST_UPCOMING_CYCLE_DETECTION
    TestUpcomingCycleDetection(
        position, distance_from_root, PinnedMask(position.Board(), position.Board().stm), checkers);
#endif

    if (!root_node && alpha < 0 && position.upcoming_rep(distance_from_root))
    {
        alpha = 0;
        if (alpha >= beta)
        {
            return alpha;
        }
    }

    // If we are in a singular move search, we don't want to do any early pruning

    // Step 3: Probe transposition table
    const auto [tt_entry, tt_score, tt_depth, tt_cutoff, tt_move, tt_eval] = probe_tt(position, distance_from_root);

    // Step 4: Check if we can use the TT entry to return early
    if (!pv_node && ss->singular_exclusion == Move::Uninitialized && tt_depth >= depth
        && tt_cutoff != SearchResultType::EMPTY && tt_score != SCORE_UNDEFINED)
    {
        if (auto value = tt_cutoff_node(position, tt_score, tt_cutoff, alpha, beta))
        {
            return *value;
        }
    }

    // Step 5: Probe syzygy EGTB
    if (ss->singular_exclusion == Move::Uninitialized)
    {
        if (auto value = probe_egtb<root_node, pv_node>(
                position, distance_from_root, local, ss, alpha, beta, min_score, max_score, depth))
        {
            return *value;
        }
    }

    const auto [raw_eval, eval] = get_search_eval<false>(
        position, ss, local, tt_entry, tt_eval, tt_score, tt_cutoff, depth, distance_from_root);

    // Step 6: Static null move pruning (a.k.a reverse futility pruning)
    //
    // If the static score is far above beta we fail high.
    if (!pv_node && !checkers && ss->singular_exclusion == Move::Uninitialized && depth < 8
        && eval - 93 * depth >= beta)
    {
        return beta;
    }

    // Step 7: Null move pruning
    //
    // If our static store is above beta, we skip a move. If the resulting position is still above beta, then we can
    // fail high assuming there is at least one move in the current position that would allow us to improve. This
    // heruistic fails in zugzwang positions, so we have a verification search.
    if (!pv_node && !checkers && ss->singular_exclusion == Move::Uninitialized && (ss - 1)->move != Move::Uninitialized
        && distance_from_root >= ss->nmp_verification_depth && eval > beta
        && !(tt_entry && tt_cutoff == SearchResultType::UPPER_BOUND && tt_score < beta))
    {
        if (auto value = null_move_pruning(position, ss, local, shared, distance_from_root, depth, eval, beta))
        {
            return *value;
        }
    }

    // Step 8: Mate distance pruning
    alpha = std::max(Score::mated_in(distance_from_root), alpha);
    beta = std::min(Score::mate_in(distance_from_root + 1), beta);
    if (alpha >= beta)
        return alpha;

    // Set up search variables
    Move bestMove = Move::Uninitialized;
    auto original_alpha = alpha;
    int seen_moves = 0;
    bool noLegalMoves = true;
    ss->threat_mask = ThreatMask(position.Board(), !position.Board().stm);

    // Step 9: Rebel style IID
    //
    // If we have reached a node where we would normally expect a TT entry but there isn't one, we reduce the search
    // depth. This fits into the iterative deepening model better and avoids the engine spending too much time searching
    // new nodes in the tree at high depths
    if (!tt_entry && depth > 3)
    {
        depth--;
    }

    const auto pinned = PinnedMask(position.Board(), position.Board().stm);
    StagedMoveGenerator gen(position, ss, local, tt_move, false, pinned, checkers);
    Move move;
    ss->cont_hist_subtables = local.cont_hist.get_subtables(ss);

    // Step 10: Iterate over each potential move until we reach the end or find a beta cutoff
    while (gen.Next(move))
    {
        if (!MoveIsLegal(position.Board(), move, pinned, checkers))
        {
            continue;
        }

        noLegalMoves = false;

        if (move == ss->singular_exclusion || (root_node && local.RootExcludeMove(move)))
        {
            continue;
        }

        seen_moves++;

        // Step 11: Late move pruning
        //
        // At low depths, we limit the number of candidate quiet moves. This is a more aggressive form of futility
        // pruning
        if (depth < 6 && seen_moves >= 3 + 3 * depth && score > Score::tb_loss_in(MAX_DEPTH))
        {
            gen.SkipQuiets();
        }

        // Step 12: Futility pruning
        //
        // Prune quiet moves if we are significantly below alpha. TODO: this implementation is a little strange
        if (!pv_node && !checkers && depth < 10 && eval + 31 + 13 * depth + 11 * depth * depth < alpha
            && score > Score::tb_loss_in(MAX_DEPTH))
        {
            gen.SkipQuiets();
            if (gen.GetStage() >= Stage::GIVE_BAD_LOUD)
            {
                break;
            }
        }

        // Step 13: SEE pruning
        //
        // If a move appears to lose material we prune it. The margin is adjusted based on depth and history. This means
        // we more aggressivly prune bad history moves, and allow good history moves even if they appear to lose
        // material.
        bool is_loud_move = move.IsCapture() || move.IsPromotion();
        int history = is_loud_move
            ? local.loud_history.get(position, ss, move)
            : (local.quiet_history.get(position, ss, move) + local.cont_hist.get(position, ss, move));

        if (score > Score::tb_loss_in(MAX_DEPTH) && !is_loud_move && depth <= 6
            && !see_ge(position.Board(), move, -111 * depth - history / 168))
        {
            continue;
        }

        int extensions = 0;

        // Step 14: Singular extensions.
        //
        // If one move is significantly better than all alternatives, we extend the search for that
        // critical move. When looking for potentially singular moves, we look for TT moves at sufficient depth with
        // an exact or lower-bound cutoff. We also avoid testing for singular moves at the root or when already
        // testing for singularity. To test for singularity, we do a reduced depth search on the TT score lowered by
        // some margin. If this search fails low, this implies all alternative moves are much worse and the TT move
        // is singular.
        if (!root_node && ss->singular_exclusion == Move::Uninitialized && depth >= 7 && tt_depth + 3 >= depth
            && tt_cutoff != SearchResultType::UPPER_BOUND && tt_move == move && tt_score != SCORE_UNDEFINED)
        {
            if (auto value
                = singular_extensions<pv_node>(position, ss, local, shared, depth, tt_score, tt_move, beta, extensions))
            {
                return *value;
            }
        }

        ss->move = move;
        ss->moved_piece = position.Board().GetSquare(move.GetFrom());
        ss->cont_hist_subtable
            = &local.cont_hist.table[position.Board().stm][GetPieceType(ss->moved_piece)][move.GetTo()];
        position.ApplyMove(move);
        tTable.PreFetch(position.Board().GetZobristKey()); // load the transposition into l1 cache. ~5% speedup
        local.net.StoreLazyUpdates(position.PrevBoard(), position.Board(), (ss + 1)->acc, move);

        // Step 15: Check extensions
        if (IsInCheck(position.Board()))
        {
            extensions += 1;
        }

        // Step 16: Late move reductions
        int r = reduction<pv_node>(depth, seen_moves, history);
        Score search_score
            = search_move<pv_node>(position, ss, local, shared, depth, extensions, r, alpha, beta, seen_moves);

        position.RevertMove();

        if (local.aborting_search)
        {
            return SCORE_UNDEFINED;
        }

        // Step 17: Update history/killer move tables and check for fail-high
        if (update_search_stats<pv_node>(ss, gen, depth, search_score, move, score, bestMove, alpha, beta))
        {
            break;
        }
    }

    // Step 18: Handle terminal node conditions
    if (noLegalMoves)
    {
        return TerminalScore(position.Board(), distance_from_root);
    }

    score = std::clamp(score, min_score, max_score);

    // Step 19: Return early when in a singular extension root search
    if (local.aborting_search || ss->singular_exclusion != Move::Uninitialized)
    {
        return score;
    }

    const auto bound = score <= original_alpha ? SearchResultType::UPPER_BOUND
        : score >= beta                        ? SearchResultType::LOWER_BOUND
                                               : SearchResultType::EXACT;

    // Step 20: Adjust eval correction history
    if (!checkers && !(bestMove.IsCapture() || bestMove.IsPromotion())
        && !(bound == SearchResultType::LOWER_BOUND && score <= raw_eval)
        && !(bound == SearchResultType::UPPER_BOUND && score >= raw_eval))
    {
        local.pawn_corr_hist.add(position, depth, score.value() - raw_eval.value());
    }

    // Step 21: Update transposition table
    tTable.AddEntry(bestMove, position.Board().GetZobristKey(), score, depth, position.Board().half_turn_count,
        distance_from_root, bound, raw_eval);

    return score;
}

template <SearchType search_type>
Score Quiescence(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared,
    int depth, Score alpha, Score beta)
{
    static_assert(search_type != SearchType::ROOT);
    assert((search_type == SearchType::PV) || (beta == alpha + 1));
    constexpr bool pv_node = search_type != SearchType::ZW;
    const auto distance_from_root = ss->distance_from_root;

    // Step 1: Check for abort or draw and update stats in the local state and search stack
    if (auto value = init_search_node(position, distance_from_root, ss, local, shared))
    {
        return *value;
    }

    if (alpha < 0 && position.upcoming_rep(distance_from_root))
    {
        alpha = 0;
        if (alpha >= beta)
        {
            return alpha;
        }
    }

    // Step 2: Probe transposition table
    const auto [tt_entry, tt_score, tt_depth, tt_cutoff, tt_move, tt_eval] = probe_tt(position, distance_from_root);

    // Step 3: Check if we can use the TT entry to return early
    if (!pv_node && ss->singular_exclusion == Move::Uninitialized && tt_depth >= depth
        && tt_cutoff != SearchResultType::EMPTY && tt_score != SCORE_UNDEFINED)
    {
        if (auto value = tt_cutoff_node(position, tt_score, tt_cutoff, alpha, beta))
        {
            return *value;
        }
    }

    const auto [raw_eval, eval]
        = get_search_eval<true>(position, ss, local, tt_entry, tt_eval, tt_score, tt_cutoff, depth, distance_from_root);

    // Step 4: Stand-pat. We assume if all captures are bad, there's at least one quiet move that maintains the static
    // score
    alpha = std::max(alpha, eval);
    if (alpha >= beta)
    {
        return alpha;
    }

    Move bestmove = Move::Uninitialized;
    auto score = eval;
    auto original_alpha = alpha;
    const auto checkers = Checkers(position.Board());

    const auto pinned = PinnedMask(position.Board(), position.Board().stm);
    StagedMoveGenerator gen(position, ss, local, tt_move, true, pinned, checkers);
    Move move;

    while (gen.Next(move))
    {
        if (!MoveIsLegal(position.Board(), move, pinned, checkers))
        {
            continue;
        }

        if (!move.IsPromotion() && !see_ge(position.Board(), move, alpha - eval - 280))
        {
            continue;
        }

        // prune underpromotions
        if (move.IsPromotion() && !(move.GetFlag() == QUEEN_PROMOTION || move.GetFlag() == QUEEN_PROMOTION_CAPTURE))
        {
            continue;
        }

        ss->move = move;
        ss->moved_piece = position.Board().GetSquare(move.GetFrom());
        ss->cont_hist_subtable
            = &local.cont_hist.table[position.Board().stm][GetPieceType(ss->moved_piece)][move.GetTo()];
        position.ApplyMove(move);
        // TODO: prefetch
        local.net.StoreLazyUpdates(position.PrevBoard(), position.Board(), (ss + 1)->acc, move);
        auto search_score = -Quiescence<search_type>(position, ss + 1, local, shared, depth - 1, -beta, -alpha);
        position.RevertMove();

        if (local.aborting_search)
        {
            return SCORE_UNDEFINED;
        }

        // Step 5: Update best score and check for fail-high
        if (update_search_stats<pv_node>(ss, gen, depth, search_score, move, score, bestmove, alpha, beta))
        {
            break;
        }
    }

    // Step 6: Return early when in a singular extension root search
    if (local.aborting_search || ss->singular_exclusion != Move::Uninitialized)
    {
        return score;
    }

    const auto bound = score <= original_alpha ? SearchResultType::UPPER_BOUND
        : score >= beta                        ? SearchResultType::LOWER_BOUND
                                               : SearchResultType::EXACT;

    // Step 7: Update transposition table
    tTable.AddEntry(bestmove, position.Board().GetZobristKey(), score, depth, position.Board().half_turn_count,
        distance_from_root, bound, raw_eval);

    return score;
}
