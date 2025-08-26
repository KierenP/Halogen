#include "search/search.h"

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

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "chessboard/game_state.h"
#include "evaluation/evaluate.h"
#include "movegen/list.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "network/network.h"
#include "search/data.h"
#include "search/history.h"
#include "search/limit/limits.h"
#include "search/limit/time.h"
#include "search/score.h"
#include "search/staged_movegen.h"
#include "search/static_exchange_evaluation.h"
#include "search/syzygy.h"
#include "search/transposition/entry.h"
#include "search/transposition/table.h"
#include "search/zobrist.h"
#include "spsa/tuneable.h"
#include "uci/uci.h"
#include "utility/atomic.h"
#include "utility/static_vector.h"

enum class SearchType : int8_t
{
    ROOT,
    PV,
    ZW,
};

void iterative_deepening(GameState& position, SearchLocalState& local, SearchSharedState& shared);

Score aspiration_window(
    GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared, Score mid_score);

template <SearchType search_type>
Score search(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared, int depth,
    Score alpha, Score beta, bool cut_node);

template <SearchType search_type>
Score qsearch(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared, int depth,
    Score alpha, Score beta);

void launch_worker_search(GameState& position, SearchLocalState& local, SearchSharedState& shared)
{
    iterative_deepening(position, local, shared);
}

void iterative_deepening(GameState& position, SearchLocalState& local, SearchSharedState& shared)
{
    auto* ss = local.search_stack.root();
    Score mid_score = 0;

    for (int depth = 1; depth < MAX_ITERATIVE_DEEPENING; depth++)
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
            auto score = aspiration_window(position, ss, local, shared, mid_score);

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

            // node based time management
            const auto idx = std::ranges::distance(
                local.root_moves.begin(), std::ranges::find(local.root_moves, ss->pv[0], &RootMove::move));
            const auto node_factor
                = node_tm_base + node_tm_scale * (1 - float(local.root_moves[idx].nodes) / float(local.nodes));

            if (shared.limits.time && !shared.limits.time->should_continue_search(node_factor))
            {
                local.thread_wants_to_stop = true;
                shared.report_thread_wants_to_stop();
            }
        }

        if (shared.limits.mate && Score::Limits::MATE - abs(mid_score.value()) <= shared.limits.mate.value() * 2)
        {
            return;
        }
    }
}

Score aspiration_window(
    GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared, Score mid_score)
{
    Score delta = aspiration_window_size;
    Score alpha = std::max<Score>(Score::Limits::MATED, mid_score - delta);
    Score beta = std::min<Score>(Score::Limits::MATE, mid_score + delta);
    int fail_high_count = 0;

    while (true)
    {
        local.sel_septh = 0;
        auto adjusted_depth = std::max(1, local.curr_depth - fail_high_count);
        auto score = search<SearchType::ROOT>(position, ss, local, shared, adjusted_depth, alpha, beta, false);

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
            fail_high_count = 0;
        }

        if (score >= beta)
        {
            score = beta;
            shared.report_search_result(ss, local, score, SearchResultType::LOWER_BOUND);
            beta = std::min<Score>(Score::Limits::MATE, beta + delta);
            fail_high_count++;
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

    if (shared.limits.time && shared.limits.time->should_abort_search())
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

    if (distance_from_root >= MAX_RECURSION)
    {
        return 0;
    }

    ss->pv.clear();

    if (insufficient_material(position.board()))
    {
        return Score::draw_random(local.nodes);
    }

    // Draw randomness as in https://github.com/Luecx/Koivisto/commit/c8f01211c290a582b69e4299400b667a7731a9f7
    if (position.is_repetition(distance_from_root))
    {
        return Score::draw_random(local.nodes);
    }

    if (position.board().fifty_move_count >= 100)
    {
        if (position.board().checkers)
        {
            BasicMoveList moves;
            legal_moves(position.board(), moves);
            if (moves.empty())
            {
                return Score::mated_in(distance_from_root);
            }
        }

        return Score::draw_random(local.nodes);
    }

    if (!ss->nmp_verification_root)
    {
        ss->nmp_verification_depth = (ss - 1)->nmp_verification_depth;
    }

    return std::nullopt;
}

template <bool root_node, bool pv_node>
std::optional<Score> probe_egtb(const GameState& position, const int distance_from_root, SearchSharedState& shared,
    SearchLocalState& local, SearchStackState* ss, Score& alpha, Score& beta, Score& min_score, Score& max_score,
    const int depth)
{
    auto probe = Syzygy::probe_wdl_search(local, distance_from_root);
    if (probe.has_value())
    {
        local.tb_hits.fetch_add(1, std::memory_order_relaxed);
        const auto tb_score = *probe;

        if constexpr (!root_node)
        {
            const auto bound = [tb_score]()
            {
                if (tb_score.is_win())
                {
                    return SearchResultType::LOWER_BOUND;
                }
                else if (tb_score.is_loss())
                {
                    return SearchResultType::UPPER_BOUND;
                }
                else
                {
                    return SearchResultType::EXACT;
                }
            }();

            if (bound == SearchResultType::EXACT || (bound == SearchResultType::LOWER_BOUND && tb_score >= beta)
                || (bound == SearchResultType::UPPER_BOUND && tb_score <= alpha))
            {
                shared.transposition_table.add_entry(Move::Uninitialized,
                    Zobrist::get_fifty_move_adj_key(position.board()), tb_score, depth,
                    position.board().half_turn_count, distance_from_root, bound, SCORE_UNDEFINED);
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
            if (tb_score.is_win())
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

std::tuple<Transposition::Entry*, Score, int, SearchResultType, Move, Score> probe_tt(
    SearchSharedState& shared, const GameState& position, const int distance_from_root)
{
    // copy the values out of the table that we want, to avoid race conditions
    const auto adjusted_key = Zobrist::get_fifty_move_adj_key(position.board());
    auto* tt_entry
        = shared.transposition_table.get_entry(adjusted_key, distance_from_root, position.board().half_turn_count);
    const auto tt_score
        = tt_entry ? Transposition::convert_from_tt_score(tt_entry->score, distance_from_root) : SCORE_UNDEFINED;
    const auto tt_depth = tt_entry ? tt_entry->depth : 0;
    const auto tt_cutoff = tt_entry ? tt_entry->meta.type : SearchResultType::EMPTY;
    const auto tt_move = tt_entry ? tt_entry->move : Move::Uninitialized;
    const auto tt_eval = tt_entry ? tt_entry->static_eval : SCORE_UNDEFINED;
    return { tt_entry, tt_score, tt_depth, tt_cutoff, tt_move, tt_eval };
}

std::optional<Score> tt_cutoff_node(const GameState& position, const Score tt_score, const SearchResultType tt_cutoff,
    const Score alpha, const Score beta)
{
    // Don't take scores from the TT if there's a two-fold repetition
    if (position.is_two_fold_repetition())
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
    return (board.get_pieces_bb(board.stm)
        == (board.get_pieces_bb(KING, board.stm) | board.get_pieces_bb(PAWN, board.stm)));
}

std::optional<Score> null_move_pruning(GameState& position, SearchStackState* ss, SearchLocalState& local,
    SearchSharedState& shared, const int distance_from_root, const int depth, const Score static_score,
    const Score beta)
{
    // avoid null move pruning in very late game positions due to zanauag issues.
    // Even with verification search e.g 8/6k1/8/8/8/8/1K6/Q7 w - - 0 1
    if (IsEndGame(position.board()) || std::popcount(position.board().get_pieces_bb()) < 5)
    {
        return std::nullopt;
    }

    const int reduction = nmp_const + depth / nmp_d + std::min(nmp_sd, (static_score - beta).value() / nmp_s);

    ss->move = Move::Uninitialized;
    ss->moved_piece = N_PIECES;
    ss->cont_hist_subtable = nullptr;
    ss->cont_corr_hist_subtable = nullptr;
    position.apply_null_move();
    // TODO: separate out the accumulator stack from search stack. Then we can make this a no-op
    local.net.store_lazy_updates(position.prev_board(), position.board(), (ss + 1)->acc, Move::Uninitialized);
    auto null_move_score
        = -search<SearchType::ZW>(position, ss + 1, local, shared, depth - reduction - 1, -beta, -beta + 1, false);
    position.revert_null_move();

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
            = search<SearchType::ZW>(position, ss, local, shared, depth - reduction - 1, beta - 1, beta, false);
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
    SearchSharedState& shared, int depth, const Score tt_score, const Move tt_move, const Score beta, int& extensions,
    bool cut_node)
{
    Score sbeta = tt_score - se_sbeta_depth * depth / 64;
    int sdepth = depth / 2;

    ss->singular_exclusion = tt_move;

    auto se_score = search<SearchType::ZW>(position, ss, local, shared, sdepth, sbeta - 1, sbeta, cut_node);

    ss->singular_exclusion = Move::Uninitialized;

    // If the TT move is singular, we extend the search by one or more plies depending on how singular it appears
    if (se_score < sbeta - se_double && !pv_node)
    {
        extensions += 2;
    }
    else if (se_score < sbeta)
    {
        extensions += 1;
    }

    // Multi-Cut: In this case, we have proven that at least one other move appears to fail high, along with
    // the TT move having a LOWER_BOUND score of significantly above beta. In this case, we can assume the node
    // will fail high and we return a soft bound. Avoid returning false mate scores.
    else if (sbeta >= beta && !sbeta.is_decisive())
    {
        return sbeta;
    }

    // Negative extensions: if the TT move is not singular, but also doesn't appear good enough to multi-cut, we
    // might decide to reduce the TT move search. The TT move doesn't have LMR applied, to heuristically this
    // reduction can be thought of as evening out the search depth between the moves and not favouring the TT
    // move as heavily.
    else if (tt_score >= beta)
    {
        extensions += -2;
    }

    else if (cut_node)
    {
        extensions += -2;
    }

    return std::nullopt;
}

template <bool pv_node>
int late_move_reduction(int depth, int seen_moves, int history, bool cut_node, bool improving, bool loud)
{
    if (seen_moves == 1)
    {
        return 0;
    }

    int r = LMR_reduction[std::clamp(depth, 0, 63)][std::clamp(seen_moves, 0, 63)];

    if constexpr (pv_node)
    {
        r -= lmr_pv;
    }

    if (cut_node)
    {
        r += lmr_cut;
    }

    if (improving)
    {
        r -= lmr_improving;
    }

    if (loud)
    {
        r -= lmr_loud;
    }

    r -= history * lmr_h / 16384;

    r = (r + lmr_offset) / LMR_SCALE;
    return std::max(0, r);
}

void UpdatePV(Move move, SearchStackState* ss)
{
    ss->pv.clear();
    ss->pv.emplace_back(move);
    ss->pv.insert(ss->pv.end(), (ss + 1)->pv.begin(), (ss + 1)->pv.end());
}

void AddHistory(const StagedMoveGenerator& gen, const Move& move, int depthRemaining)
{
    const auto bonus = (history_bonus_const + history_bonus_depth * depthRemaining
                           + history_bonus_quad * depthRemaining * depthRemaining)
        / 64;
    const auto penalty = -(history_penalty_const + history_penalty_depth * depthRemaining
                             + history_penalty_quad * depthRemaining * depthRemaining)
        / 64;

    if (move.is_capture() || move.is_promotion())
    {
        gen.update_loud_history(move, bonus, penalty);
    }
    else
    {
        gen.update_quiet_history(move, bonus, penalty);
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
    const int seen_moves, bool cut_node)
{
    const int new_depth = depth + extensions - 1;
    Score search_score = 0;

    if (reductions > 0)
    {
        search_score = -search<SearchType::ZW>(
            position, ss + 1, local, shared, new_depth - reductions, -(alpha + 1), -alpha, true);

        if (search_score <= alpha)
        {
            return search_score;
        }
    }

    // If the reduced depth search was skipped or failed high, we do a full depth zero width search
    if (!pv_node || seen_moves > 1)
    {
        search_score
            = -search<SearchType::ZW>(position, ss + 1, local, shared, new_depth, -(alpha + 1), -alpha, !cut_node);
    }

    // If the ZW search was skipped or failed high, we do a full depth full width search
    if (pv_node && (seen_moves == 1 || search_score > alpha))
    {
        search_score = -search<SearchType::PV>(position, ss + 1, local, shared, new_depth, -beta, -alpha, false);
    }

    return search_score;
}

// { raw, adjusted }
template <bool is_qsearch>
std::tuple<Score, Score> get_search_eval(const GameState& position, SearchStackState* ss, SearchSharedState& shared,
    SearchLocalState& local, Transposition::Entry* const tt_entry, const Score tt_eval, const Score tt_score,
    const SearchResultType tt_cutoff, int depth, int distance_from_root, bool in_check)
{
    if (in_check)
    {
        ss->adjusted_eval = (ss - 2)->adjusted_eval;
        return { SCORE_UNDEFINED, SCORE_UNDEFINED };
    }

    Score raw_eval;
    Score adjusted_eval;

    auto scale_eval_50_move = [&position](Score eval)
    {
        // rescale and skew the raw eval based on the 50 move rule. We need to reclamp the score to ensure we don't
        // return false mate scores
        return eval.value() * (fifty_mr_scale_a - (int)position.board().fifty_move_count) / fifty_mr_scale_b;
    };

    auto eval_corr_history = [&](Score eval)
    {
        eval += local.pawn_corr_hist.get_correction_score(position);
        eval += local.non_pawn_corr[WHITE].get_correction_score(position, WHITE);
        eval += local.non_pawn_corr[BLACK].get_correction_score(position, BLACK);

        if ((ss - 2)->cont_corr_hist_subtable)
        {
            eval += (ss - 2)->cont_corr_hist_subtable->get_correction_score(position, ss);
        }
        return eval;
    };

    if (tt_entry)
    {
        if (tt_eval != SCORE_UNDEFINED)
        {
            raw_eval = tt_eval;
        }
        else
        {
            tt_entry->static_eval = raw_eval = evaluate(position.board(), ss, local.net);
        }

        adjusted_eval = scale_eval_50_move(raw_eval);
        adjusted_eval = eval_corr_history(adjusted_eval);
        ss->adjusted_eval = adjusted_eval;

        // Use the tt_score to improve the static eval if possible. Avoid returning unproved mate scores in q-search
        if (tt_score != SCORE_UNDEFINED && (!is_qsearch || !tt_score.is_decisive())
            && (tt_cutoff == SearchResultType::EXACT
                || (tt_cutoff == SearchResultType::LOWER_BOUND && tt_score >= adjusted_eval)
                || (tt_cutoff == SearchResultType::UPPER_BOUND && tt_score <= adjusted_eval)))
        {
            adjusted_eval = tt_score;
        }
    }
    else
    {
        raw_eval = evaluate(position.board(), ss, local.net);
        adjusted_eval = scale_eval_50_move(raw_eval);
        adjusted_eval = eval_corr_history(adjusted_eval);
        ss->adjusted_eval = adjusted_eval;
        shared.transposition_table.add_entry(Move::Uninitialized, Zobrist::get_fifty_move_adj_key(position.board()),
            SCORE_UNDEFINED, depth, position.board().half_turn_count, distance_from_root, SearchResultType::EMPTY,
            raw_eval);
    }

    adjusted_eval = std::clamp<Score>(adjusted_eval, Score::Limits::EVAL_MIN, Score::Limits::EVAL_MAX);
    return { raw_eval, adjusted_eval };
}

void test_upcoming_cycle_detection(GameState& position, int distance_from_root)
{
    // upcoming_rep should return true iff there is a legal move that will lead to a repetition.
    // It's possible to have a zobrist hash collision, so this isn't a perfect test. But the likelihood of this
    // happening is very low.

    bool has_upcoming_rep = position.upcoming_rep(distance_from_root);
    bool is_draw = false;
    BasicMoveList moves;
    legal_moves(position.board(), moves);

    for (const auto& move : moves)
    {
        position.apply_move(move);
        if (position.is_repetition(distance_from_root + 1))
        {
            is_draw = true;
        }
        position.revert_move();
    }

    if (has_upcoming_rep != is_draw)
    {
        std::cout << "Upcoming rep: " << has_upcoming_rep << " != " << is_draw << std::endl;
        std::cout << position.board() << std::endl;
        position.upcoming_rep(distance_from_root);
    }
}

template <SearchType search_type>
Score search(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared, int depth,
    Score alpha, Score beta, bool cut_node)
{
    assert((search_type != SearchType::ROOT) || (ss->distance_from_root == 0));
    assert((search_type != SearchType::ZW) || (beta == alpha + 1));
    constexpr bool pv_node = search_type != SearchType::ZW;
    constexpr bool root_node = search_type == SearchType::ROOT;
    assert(!(pv_node && cut_node));
    [[maybe_unused]] const bool allNode = !(pv_node || cut_node);
    const auto distance_from_root = ss->distance_from_root;
    const bool InCheck = position.board().checkers;

    // Step 1: Drop into q-search
    if (depth <= 0)
    {
        constexpr SearchType qsearch_type = pv_node ? SearchType::PV : SearchType::ZW;
        return qsearch<qsearch_type>(position, ss, local, shared, depth, alpha, beta);
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
    test_upcoming_cycle_detection(position, distance_from_root);
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
    const auto [tt_entry, tt_score, tt_depth, tt_cutoff, tt_move, tt_eval]
        = probe_tt(shared, position, distance_from_root);

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
                position, distance_from_root, shared, local, ss, alpha, beta, min_score, max_score, depth))
        {
            return *value;
        }
    }

    const auto [raw_eval, eval] = get_search_eval<false>(
        position, ss, shared, local, tt_entry, tt_eval, tt_score, tt_cutoff, depth, distance_from_root, InCheck);
    const bool improving = ss->adjusted_eval > (ss - 2)->adjusted_eval;

    // Step 6: Static null move pruning (a.k.a reverse futility pruning)
    //
    // If the static score is far above beta we fail high.
    const bool has_active_threat = position.board().active_lesser_threats();
    const auto rfp_margin
        = (rfp_const + rfp_depth * (depth - improving) + rfp_quad * (depth - improving) * (depth - improving)) / 64;
    if (!pv_node && !InCheck && ss->singular_exclusion == Move::Uninitialized && depth < rfp_max_d
        && eval - rfp_margin - rfp_threat * has_active_threat >= beta)
    {
        return (beta.value() + eval.value()) / 2;
    }

    // Step 7: Null move pruning
    //
    // If our static store is above beta, we skip a move. If the resulting position is still above beta, then we can
    // fail high assuming there is at least one move in the current position that would allow us to improve. This
    // heruistic fails in zugzwang positions, so we have a verification search.
    if (!pv_node && !InCheck && ss->singular_exclusion == Move::Uninitialized && (ss - 1)->move != Move::Uninitialized
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

    // Step 9: Rebel style IID
    //
    // If we have reached a node where we would normally expect a TT entry but there isn't one, we reduce the search
    // depth. This fits into the iterative deepening model better and avoids the engine spending too much time searching
    // new nodes in the tree at high depths
    if (!tt_entry && depth > iid_depth)
    {
        depth--;
    }

    StagedMoveGenerator gen(position, ss, local, tt_move);
    Move move;

    // Step 10: Iterate over each potential move until we reach the end or find a beta cutoff
    while (gen.next(move))
    {
        if (move == ss->singular_exclusion || (root_node && local.should_skip_root_move(move)))
        {
            continue;
        }

        noLegalMoves = false;
        const int64_t prev_nodes = local.nodes;
        seen_moves++;

        // Step 11: Late move pruning
        //
        // At low depths, we limit the number of candidate quiet moves. This is a more aggressive form of futility
        // pruning
        const auto lmp_margin
            = (lmp_const + lmp_depth * depth * (1 + improving) + lmp_quad * depth * depth * (1 + improving)) / 64;
        if (depth < lmp_max_d && seen_moves >= lmp_margin && !score.is_loss())
        {
            gen.skip_quiets();
        }

        // Step 12: Futility pruning
        //
        // Prune quiet moves if we are significantly below alpha. TODO: this implementation is a little strange
        if (!pv_node && !InCheck && depth < fp_max_d
            && eval + (fp_const + fp_depth * depth + fp_quad * depth * depth) / 64 < alpha && !score.is_loss())
        {
            gen.skip_quiets();
            if (gen.get_stage() >= Stage::GIVE_BAD_LOUD)
            {
                break;
            }
        }

        // Step 13: SEE pruning
        //
        // If a move appears to lose material we prune it. The margin is adjusted based on depth and history. This means
        // we more aggressivly prune bad history moves, and allow good history moves even if they appear to lose
        // material.
        bool is_loud_move = move.is_capture() || move.is_promotion();
        int history = is_loud_move ? local.get_loud_history(ss, move) : (local.get_quiet_search_history(ss, move));
        auto see_pruning_margin = is_loud_move ? -see_loud_depth * depth * depth - history / see_loud_hist
                                               : -see_quiet_depth * depth - history / see_quiet_hist;

        if (!score.is_loss() && depth <= see_max_depth && !see_ge(position.board(), move, see_pruning_margin))
        {
            continue;
        }

        if (!score.is_loss() && !is_loud_move && history < -hist_prune_depth * depth - hist_prune)
        {
            gen.skip_quiets();
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
        if (!root_node && ss->singular_exclusion == Move::Uninitialized && depth >= se_min_depth
            && tt_depth + se_tt_depth >= depth && tt_cutoff != SearchResultType::UPPER_BOUND && tt_move == move
            && tt_score != SCORE_UNDEFINED)
        {
            if (auto value = singular_extensions<pv_node>(
                    position, ss, local, shared, depth, tt_score, tt_move, beta, extensions, cut_node))
            {
                return *value;
            }
        }

        ss->move = move;
        ss->moved_piece = position.board().get_square_piece(move.from());
        ss->cont_hist_subtable
            = &local.cont_hist.table[position.board().stm][enum_to<PieceType>(ss->moved_piece)][move.to()];
        ss->cont_corr_hist_subtable
            = &local.cont_corr_hist.table[position.board().stm][enum_to<PieceType>(ss->moved_piece)][move.to()];
        position.apply_move(move);
        shared.transposition_table.prefetch(
            Zobrist::get_fifty_move_adj_key(position.board())); // load the transposition into l1 cache. ~5% speedup
        local.net.store_lazy_updates(position.prev_board(), position.board(), (ss + 1)->acc, move);

        // Step 15: Check extensions
        if (position.board().checkers)
        {
            extensions += 1;
        }

        // Step 16: Late move reductions
        int r = late_move_reduction<pv_node>(depth, seen_moves, history, cut_node, improving, is_loud_move);
        Score search_score = search_move<pv_node>(
            position, ss, local, shared, depth, extensions, r, alpha, beta, seen_moves, cut_node);

        position.revert_move();

        if (local.aborting_search)
        {
            return SCORE_UNDEFINED;
        }

        if (root_node)
        {
            const auto idx = std::ranges::distance(
                local.root_moves.begin(), std::ranges::find(local.root_moves, move, &RootMove::move));
            local.root_moves[idx].nodes += local.nodes - prev_nodes;
            local.root_moves[idx].score = search_score;
        }

        // Step 17: Update history move tables and check for fail-high
        if (update_search_stats<pv_node>(ss, gen, depth, search_score, move, score, bestMove, alpha, beta))
        {
            break;
        }
    }

    // Step 18: Handle terminal node conditions
    if (noLegalMoves)
    {
        if (ss->singular_exclusion != Move::Uninitialized)
        {
            return alpha;
        }
        else if (InCheck)
        {
            return Score::mated_in(distance_from_root);
        }
        else
        {
            return Score::draw_random(local.nodes);
        }
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
    if (!InCheck && !(bestMove.is_capture() || bestMove.is_promotion())
        && !(bound == SearchResultType::LOWER_BOUND && score <= ss->adjusted_eval)
        && !(bound == SearchResultType::UPPER_BOUND && score >= ss->adjusted_eval))
    {
        const auto adj = score.value() - ss->adjusted_eval.value();
        local.pawn_corr_hist.add(position, depth, adj);
        local.non_pawn_corr[WHITE].add(position, WHITE, depth, adj);
        local.non_pawn_corr[BLACK].add(position, BLACK, depth, adj);
        if ((ss - 2)->cont_corr_hist_subtable)
        {
            (ss - 2)->cont_corr_hist_subtable->add(position, ss, depth, adj);
        }
    }

    // Step 21: Update transposition table
    shared.transposition_table.add_entry(bestMove, Zobrist::get_fifty_move_adj_key(position.board()), score, depth,
        position.board().half_turn_count, distance_from_root, bound, raw_eval);

    return score;
}

template <SearchType search_type>
Score qsearch(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared, int depth,
    Score alpha, Score beta)
{
    static_assert(search_type != SearchType::ROOT);
    assert((search_type == SearchType::PV) || (beta == alpha + 1));
    constexpr bool pv_node = search_type != SearchType::ZW;
    const auto distance_from_root = ss->distance_from_root;
    assert(ss->singular_exclusion == Move::Uninitialized);

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
    const auto [tt_entry, tt_score, tt_depth, tt_cutoff, tt_move, tt_eval]
        = probe_tt(shared, position, distance_from_root);

    // Step 3: Check if we can use the TT entry to return early
    if (!pv_node && ss->singular_exclusion == Move::Uninitialized && tt_depth >= depth
        && tt_cutoff != SearchResultType::EMPTY && tt_score != SCORE_UNDEFINED)
    {
        if (auto value = tt_cutoff_node(position, tt_score, tt_cutoff, alpha, beta))
        {
            return *value;
        }
    }

    const bool in_check = position.board().checkers;
    const auto [raw_eval, eval] = get_search_eval<true>(
        position, ss, shared, local, tt_entry, tt_eval, tt_score, tt_cutoff, depth, distance_from_root, in_check);
    auto score = in_check ? std::numeric_limits<Score>::min() : eval;

    // Step 4: Stand-pat. We assume if all captures are bad, there's at least one quiet move that maintains the static
    // score
    if (!in_check)
    {
        alpha = std::max(alpha, eval);
        if (alpha >= beta)
        {
            return alpha;
        }
    }

    Move bestmove = Move::Uninitialized;
    auto original_alpha = alpha;
    bool no_legal_moves = true;
    int seen_moves = 0;
    StagedMoveGenerator gen(position, ss, local, tt_move, !in_check);
    Move move;

    while (gen.next(move))
    {
        no_legal_moves = false;

        if (!score.is_loss() && seen_moves >= 2)
        {
            break;
        }

        bool is_loud_move = move.is_capture() || move.is_promotion();

        if (!score.is_loss() && !is_loud_move
            && !see_ge(position.board(), move, local.get_quiet_search_history(ss, move) / 168))
        {
            continue;
        }

        seen_moves++;

        ss->move = move;
        ss->moved_piece = position.board().get_square_piece(move.from());
        ss->cont_hist_subtable
            = &local.cont_hist.table[position.board().stm][enum_to<PieceType>(ss->moved_piece)][move.to()];
        ss->cont_corr_hist_subtable
            = &local.cont_corr_hist.table[position.board().stm][enum_to<PieceType>(ss->moved_piece)][move.to()];
        position.apply_move(move);
        // TODO: prefetch
        local.net.store_lazy_updates(position.prev_board(), position.board(), (ss + 1)->acc, move);
        auto search_score = -qsearch<search_type>(position, ss + 1, local, shared, depth - 1, -beta, -alpha);
        position.revert_move();

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

    // Step 6: Handle terminal node conditions
    if (no_legal_moves && in_check)
    {
        return Score::mated_in(distance_from_root);
    }

    if (local.aborting_search)
    {
        return score;
    }

    if (score >= beta)
    {
        score = (score.value() + beta.value()) / 2;
    }

    const auto bound = score <= original_alpha ? SearchResultType::UPPER_BOUND
        : score >= beta                        ? SearchResultType::LOWER_BOUND
                                               : SearchResultType::EXACT;

    // Step 7: Update transposition table
    shared.transposition_table.add_entry(bestmove, Zobrist::get_fifty_move_adj_key(position.board()), score, depth,
        position.board().half_turn_count, distance_from_root, bound, raw_eval);

    return score;
}
