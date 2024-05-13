#include "Search.h"

#include <algorithm>
#include <array>
#include <assert.h>
#include <atomic>
#include <cmath>
#include <ctime>
#include <iostream>
#include <limits>
#include <thread>
#include <vector>

#include "BitBoardDefine.h"
#include "EGTB.h"
#include "EvalNet.h"
#include "GameState.h"
#include "MoveGeneration.h"
#include "MoveList.h"
#include "Score.h"
#include "SearchData.h"
#include "StagedMoveGenerator.h"
#include "TTEntry.h"
#include "TimeManage.h"
#include "TranspositionTable.h"
#include "Zobrist.h"

enum class SearchType
{
    ROOT,
    PV,
    ZW,
};

// [depth][move number]
const std::array<std::array<int, 64>, 64> LMR_reduction = []
{
    std::array<std::array<int, 64>, 64> ret = {};

    for (size_t i = 0; i < ret.size(); i++)
    {
        for (size_t j = 0; j < ret[i].size(); j++)
        {
            ret[i][j] = static_cast<int>(std::round(LMR_constant + LMR_depth_coeff * log(i + 1)
                + LMR_move_coeff * log(j + 1) + LMR_depth_move_coeff * log(i + 1) * log(j + 1)));
        }
    }

    return ret;
}();

template <SearchType search_type>
SearchResult NegaScout(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared,
    int depth, Score alpha, Score beta, bool allowedNull);

template <SearchType search_type>
SearchResult Quiescence(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared,
    int depth, Score alpha, Score beta);

void PrintBestMove(Move Best, const BoardState& board, bool chess960);
bool UseTransposition(Score tt_score, EntryType cutoff, Score alpha, Score beta);
bool CheckForRep(const GameState& position, int distanceFromRoot);
bool AllowedNull(bool allowedNull, const BoardState& board, bool InCheck);
bool IsEndGame(const BoardState& board);
void AddScoreToTable(Score score, Score alphaOriginal, const BoardState& board, int depthRemaining,
    int distanceFromRoot, Score beta, Move bestMove);
Score TerminalScore(const BoardState& board, int distanceFromRoot);
int extension(const BoardState& board);
void AddKiller(Move move, std::array<Move, 2>& KillerMoves);
void AddHistory(const StagedMoveGenerator& gen, const Move& move, int depthRemaining);
void UpdatePV(Move move, SearchStackState* ss);
int Reduction(int depth, int i);

void SearchPosition(GameState& position, SearchSharedState& shared, unsigned int thread_id);
SearchResult AspirationWindowSearch(unsigned int thread_id, GameState& position, SearchStackState* ss,
    SearchLocalState& local, SearchSharedState& shared, int depth, Score mid_score);

bool should_abort_search(SearchLocalState& local, const SearchSharedState& shared);

void SearchThread(GameState& position, SearchSharedState& shared)
{
    shared.ResetNewSearch();

    // Probe TB at root
    auto probe = Syzygy::probe_dtz_root(position.Board());
    BasicMoveList root_move_whitelist;
    if (probe.has_value())
    {
        root_move_whitelist = probe->root_move_whitelist;
    }

    // Limit the MultiPV setting to be at most the number of legal moves
    auto multi_pv = shared.multi_pv;
    BasicMoveList moves;
    moves.clear();
    LegalMoves(position.Board(), moves);
    shared.multi_pv = std::min<int>(shared.multi_pv, moves.size());

    KeepSearching = true;

    // TODO: in single threaded search, we can avoid the latency hit of creating a new thread.

    std::vector<std::thread> threads;

    for (int i = 0; i < shared.get_thread_count(); i++)
    {
        shared.get_local_state(i).root_move_whitelist = root_move_whitelist;
        threads.emplace_back(std::thread([position, &shared, i]() mutable { SearchPosition(position, shared, i); }));
    }

    for (size_t i = 0; i < threads.size(); i++)
    {
        threads[i].join();
    }

    // restore the MultiPV setting
    shared.multi_pv = multi_pv;

    const auto& search_result = shared.get_best_search_result();
    PrintBestMove(search_result.best_move, position.Board(), shared.chess_960);
}

void PrintBestMove(Move Best, const BoardState& board, bool chess960)
{
    std::cout << "bestmove ";

    if (chess960)
    {
        Best.Print960(board.stm, board.castle_squares);
    }
    else
    {
        Best.Print();
    }

    std::cout << std::endl;
}

void SearchPosition(GameState& position, SearchSharedState& shared, unsigned int thread_id)
{
    auto& local = shared.get_local_state(thread_id);
    auto* ss = local.search_stack.root();
    Score mid_score = 0;

    for (int depth = 1; depth < MAX_DEPTH; depth++)
    {
        if (shared.limits.HitDepthLimit(depth))
        {
            return;
        }

        if (shared.limits.HitNodeLimit(local.nodes.load(std::memory_order_relaxed)))
        {
            return;
        }

        if (!shared.limits.ShouldContinueSearch())
        {
            shared.report_thread_wants_to_stop(thread_id);
        }

        if (depth > 1 && shared.limits.HitTimeLimit())
        {
            return;
        }

        // copy the MultiPV exclusion
        local.root_move_blacklist = shared.get_multi_pv_excluded_moves();
        local.search_depth = depth;
        SearchResult result = AspirationWindowSearch(thread_id, position, ss, local, shared, depth, mid_score);
        mid_score = result.GetScore();

        // Else, if we aborted then we should return
        if (local.aborting_search)
        {
            return;
        }

        shared.report_search_result(
            thread_id, position, ss, local, depth, result, SearchSharedState::SearchInfoType::EXACT);

        if (shared.limits.HitMateLimit(result.GetScore()))
        {
            return;
        }
    }
}

SearchResult AspirationWindowSearch(unsigned int thread_id, GameState& position, SearchStackState* ss,
    SearchLocalState& local, SearchSharedState& shared, int depth, Score mid_score)
{
    Score delta = aspiration_window_mid_width;
    Score alpha = std::max<Score>(Score::Limits::MATED, mid_score - delta);
    Score beta = std::min<Score>(Score::Limits::MATE, mid_score + delta);

    while (true)
    {
        local.sel_septh = 0;
        auto result = NegaScout<SearchType::ROOT>(position, ss, local, shared, depth, alpha, beta, false);

        if (local.aborting_search)
        {
            return SCORE_UNDEFINED;
        }

        if (alpha < result.GetScore() && result.GetScore() < beta)
        {
            return result;
        }

        if (result.GetScore() <= alpha)
        {
            shared.report_search_result(
                thread_id, position, ss, local, depth, result, SearchSharedState::SearchInfoType::UPPER_BOUND);
            // Bring down beta on a fail low
            beta = (alpha + beta) / 2;
            alpha = std::max<Score>(Score::Limits::MATED, alpha - delta);
        }

        if (result.GetScore() >= beta)
        {
            shared.report_search_result(
                thread_id, position, ss, local, depth, result, SearchSharedState::SearchInfoType::LOWER_BOUND);
            beta = std::min<Score>(Score::Limits::MATE, beta + delta);
        }

        delta = delta + delta / 2;
    }
}

template <SearchType search_type>
SearchResult NegaScout(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared,
    int depth, Score alpha, Score beta, bool allowedNull)
{
    assert((search_type != SearchType::ROOT) || (ss->distance_from_root == 0));
    assert((search_type != SearchType::ZW) || (beta == alpha + 1));
    constexpr bool pv_node = search_type != SearchType::ZW;
    constexpr bool root_node = search_type == SearchType::ROOT;
    const auto distance_from_root = ss->distance_from_root;

    // check if we should abort the search
    if (should_abort_search(local, shared))
    {
        return SCORE_UNDEFINED;
    }

    local.sel_septh = std::max(local.sel_septh, distance_from_root);
    local.nodes.fetch_add(1, std::memory_order_relaxed);

    if (distance_from_root >= MAX_DEPTH)
        return 0; // Have we reached max depth?

    if constexpr (pv_node)
    {
        ss->pv.clear();
    }

    ss->multiple_extensions = (ss - 1)->multiple_extensions;

    if (DeadPosition(position.Board()))
        return 0; // Is this position a dead draw?
    if (CheckForRep(position, distance_from_root) // Have we had a draw by repitition?
        || position.Board().fifty_move_count
            > 100) // cannot use >= as it could currently be checkmate which would count as a win
        return 8
            - (local.nodes.load(std::memory_order_relaxed)
                & 0b1111); // as in https://github.com/Luecx/Koivisto/commit/c8f01211c290a582b69e4299400b667a7731a9f7
                           // with permission from Koivisto authors.

    auto score = std::numeric_limits<Score>::min();
    auto max_score = std::numeric_limits<Score>::max();
    auto min_score = std::numeric_limits<Score>::min();

    // If we are in a singular move search, we don't want to do any early pruning.

    // Probe TB in search
    if (ss->singular_exclusion == Move::Uninitialized)
    {
        auto probe = Syzygy::probe_wdl_search(position.Board());
        if (probe.has_value())
        {
            local.tb_hits.fetch_add(1, std::memory_order_relaxed);
            const auto tb_score = probe->get_score(distance_from_root);

            if (!root_node)
            {
                if (tb_score == 0)
                    return tb_score;
                if (tb_score >= Score::tb_win_in(MAX_DEPTH) && tb_score >= beta)
                    return tb_score;
                if (tb_score <= Score::tb_loss_in(MAX_DEPTH) && tb_score <= alpha)
                    return tb_score;
            }

            // Why update score ?
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
                }
                else
                {
                    max_score = tb_score;
                }
            }
        }
    }

    // copy the values out of the table that we want, to avoid race conditions
    auto tt_entry
        = tTable.GetEntry(position.Board().GetZobristKey(), distance_from_root, position.Board().half_turn_count);
    const auto tt_score = tt_entry
        ? convert_from_tt_score(tt_entry->score.load(std::memory_order_relaxed), distance_from_root)
        : SCORE_UNDEFINED;
    const auto tt_depth = tt_entry ? tt_entry->depth.load(std::memory_order_relaxed) : 0;
    const auto tt_cutoff = tt_entry ? tt_entry->cutoff.load(std::memory_order_relaxed) : EntryType::EMPTY_ENTRY;
    const auto tt_move = tt_entry ? tt_entry->move.load(std::memory_order_relaxed) : Move::Uninitialized;

    // Check if we can abort early and return this tt_entry score
    if (!pv_node && ss->singular_exclusion == Move::Uninitialized)
    {
        if (tt_entry && tt_depth >= depth)
        {
            // Don't take scores from the TT if there's a two-fold repitition
            if (!position.CheckForRep(distance_from_root, 2) && UseTransposition(tt_score, tt_cutoff, alpha, beta))
                return SearchResult(tt_score, tt_move);
        }
    }

    bool InCheck = IsInCheck(position.Board());

    // Drop into quiescence search
    if (depth <= 0 && !InCheck)
    {
        constexpr SearchType qsearch_type = pv_node ? SearchType::PV : SearchType::ZW;
        return Quiescence<qsearch_type>(position, ss, local, shared, depth, alpha, beta);
    }

    auto staticScore = EvaluatePositionNet(position, local.eval_cache);

    // Static null move pruning
    if (!pv_node && ss->singular_exclusion == Move::Uninitialized && depth < SNMP_depth
        && staticScore - SNMP_coeff * depth >= beta && !InCheck)
        return beta;

    // Null move pruning
    if (!pv_node && ss->singular_exclusion == Move::Uninitialized && AllowedNull(allowedNull, position.Board(), InCheck)
        && (staticScore > beta))
    {
        unsigned int reduction = Null_constant + depth / Null_depth_quotent
            + std::min(3, (staticScore.value() - beta.value()) / Null_beta_quotent);

        ss->move = Move::Uninitialized;
        position.ApplyNullMove();
        auto null_move_score = -NegaScout<SearchType::ZW>(
            position, ss + 1, local, shared, depth - reduction - 1, -beta, -beta + 1, false)
                                    .GetScore();
        position.RevertNullMove();

        if (null_move_score >= beta)
        {
            if (beta < Score::mated_in(MAX_DEPTH) || depth >= 10) // TODO: I'm not sure about this first condition
            {
                // Do verification search for high depths
                SearchResult result = NegaScout<SearchType::ZW>(
                    position, ss, local, shared, depth - reduction - 1, beta - 1, beta, false);
                if (result.GetScore() >= beta)
                    return result;
            }
            else
            {
                return beta;
            }
        }
    }

    // mate distance pruning
    alpha = std::max(Score::mated_in(distance_from_root), alpha);
    beta = std::min(Score::mate_in(distance_from_root), beta);
    if (alpha >= beta)
        return alpha;

    // Set up search variables
    Move bestMove = Move::Uninitialized;
    auto original_alpha = alpha;
    int seen_moves = 0;
    bool noLegalMoves = true;

    // Rebel style IID. Don't ask why this helps but it does.
    if (!tt_entry && depth > 3)
        depth--;

    bool FutileNode = depth < Futility_depth && staticScore + Futility_constant + Futility_coeff * depth < alpha;

    StagedMoveGenerator gen(position, ss, local, tt_move, false);
    Move move;

    while (gen.Next(move))
    {
        noLegalMoves = false;

        if (move == ss->singular_exclusion)
        {
            continue;
        }

        if (root_node && local.RootExcludeMove(move))
        {
            continue;
        }

        seen_moves++;

        // late move pruning
        if (depth < LMP_depth && seen_moves >= LMP_constant + LMP_coeff * depth && score > Score::tb_loss_in(MAX_DEPTH))
        {
            gen.SkipQuiets();
        }

        // futility pruning
        if (!pv_node && FutileNode && !InCheck && score > Score::tb_loss_in(MAX_DEPTH))
        {
            gen.SkipQuiets();
            if (gen.GetStage() >= Stage::GIVE_BAD_LOUD)
            {
                break;
            }
        }

        int extensions = 0;

        // Singular extensions.
        //
        // If one move is significantly better than all alternatives, we extend the search for that
        // critical move. When looking for potentially singular moves, we look for TT moves at sufficient depth with
        // an exact or lower-bound cutoff. We also avoid testing for singular moves at the root or when already
        // testing for singularity. To test for singularity, we do a reduced depth search on the TT score lowered by
        // some margin. If this search fails low, this implies all alternative moves are much worse and the TT move
        // is singular.
        if (!root_node && ss->singular_exclusion == Move::Uninitialized && depth >= 6 && tt_entry
            && tt_depth + 2 >= depth && tt_cutoff != EntryType::UPPERBOUND && tt_move == move)
        {
            Score sbeta = tt_score - depth * 2;
            int sdepth = depth / 2;

            ss->singular_exclusion = move;

            auto result = NegaScout<SearchType::ZW>(position, ss, local, shared, sdepth, sbeta - 1, sbeta, true);

            ss->singular_exclusion = Move::Uninitialized;

            // Extending the SE idea, if the score is far below sbeta we extend by two. To avoid extending too much down
            // forced lines we limit the number of multiple_extensions down one line. We focus on non_pv nodes becuase
            // in particular we want to verify cut nodes which rest on a single good move and ensure we haven't
            // overlooked a potential non-pv line.
            if (!pv_node && result.GetScore() < sbeta - 16 && ss->multiple_extensions < 8)
            {
                extensions += 2;
                ss->multiple_extensions++;
            }
            else if (result.GetScore() < sbeta)
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
        }

        int history = local.history.get(position, ss, move);

        ss->move = move;
        position.ApplyMove(move);
        tTable.PreFetch(position.Board().GetZobristKey()); // load the transposition into l1 cache. ~5% speedup

        if (IsInCheck(position.Board()))
        {
            extensions += 1;
        }

        int reduction = 0;
        Score search_score = 0;

        // late move reductions
        if (seen_moves > 3)
        {
            reduction = Reduction(depth, seen_moves);

            if constexpr (pv_node)
                reduction--;

            reduction -= history / 4096;

            reduction = std::max(0, reduction);
        }

        const int new_depth = depth + extensions - 1;
        bool full_search = true;

        // If we are reducing, we do a zero width reduced depth search
        if (reduction > 0)
        {
            search_score = -NegaScout<SearchType::ZW>(
                position, ss + 1, local, shared, new_depth - reduction, -(alpha + 1), -alpha, true)
                                .GetScore();

            if (search_score <= alpha)
            {
                full_search = false;
            }
        }

        // If the reduced depth search was skipped or failed high, we do a full depth zero width search
        if (full_search && (!pv_node || seen_moves > 1))
        {
            search_score
                = -NegaScout<SearchType::ZW>(position, ss + 1, local, shared, new_depth, -(alpha + 1), -alpha, true)
                       .GetScore();
        }

        // If the ZW search was skipped or failed high, we do a full depth full width search
        if (full_search && pv_node && (seen_moves == 1 || (search_score > alpha && search_score < beta)))
        {
            search_score = -NegaScout<SearchType::PV>(position, ss + 1, local, shared, new_depth, -beta, -alpha, true)
                                .GetScore();
        }

        position.RevertMove();

        if (local.aborting_search)
        {
            return SCORE_UNDEFINED;
        }

        if (search_score > score)
        {
            bestMove = move;
            score = search_score;

            if (score > alpha)
            {
                alpha = score;

                if constexpr (pv_node)
                {
                    UpdatePV(move, ss);
                }

                if (alpha >= beta)
                {
                    AddKiller(move, ss->killers);
                    AddHistory(gen, move, depth);
                    break;
                }
            }
        }
    }

    // Checkmate or stalemate
    if (noLegalMoves)
    {
        return TerminalScore(position.Board(), distance_from_root);
    }

    score = std::clamp(score, min_score, max_score);

    // avoid adding scores to the TT when we are aborting the search, or during a singular extension
    if (!local.aborting_search && ss->singular_exclusion == Move::Uninitialized)
    {
        AddScoreToTable(score, original_alpha, position.Board(), depth, distance_from_root, beta, bestMove);
    }

    return SearchResult(score, bestMove);
}

int Reduction(int depth, int i)
{
    return LMR_reduction[std::min(63, std::max(0, depth))][std::min(63, std::max(0, i))];
}

void UpdatePV(Move move, SearchStackState* ss)
{
    ss->pv.clear();
    ss->pv.emplace_back(move);
    ss->pv.insert(ss->pv.end(), (ss + 1)->pv.begin(), (ss + 1)->pv.end());
}

bool UseTransposition(Score tt_score, EntryType cutoff, Score alpha, Score beta)
{
    if (cutoff == EntryType::EXACT)
    {
        return true;
    }

    if (cutoff == EntryType::LOWERBOUND && tt_score >= beta)
    {
        return true;
    }

    if (cutoff == EntryType::UPPERBOUND && tt_score <= alpha)
    {
        return true;
    }

    return false;
}

bool CheckForRep(const GameState& position, int distanceFromRoot)
{
    return position.CheckForRep(distanceFromRoot, 3);
}

int extension(const BoardState& board)
{
    int extension = 0;

    if (IsInCheck(board))
        extension += 1;

    return extension;
}

bool AllowedNull(bool allowedNull, const BoardState& board, bool InCheck)
{
    // avoid null move pruning in very late game positions due to zanauag issues.
    // Even with verification search e.g 8/6k1/8/8/8/8/1K6/Q7 w - - 0 1
    return allowedNull && !InCheck && !IsEndGame(board) && GetBitCount(board.GetAllPieces()) >= 5;
}

bool IsEndGame(const BoardState& board)
{
    return (
        board.GetPiecesColour(board.stm) == (board.GetPieceBB(KING, board.stm) | board.GetPieceBB(PAWN, board.stm)));
}

void AddScoreToTable(Score score, Score alphaOriginal, const BoardState& board, int depthRemaining,
    int distanceFromRoot, Score beta, Move bestMove)
{
    if (score <= alphaOriginal)
        tTable.AddEntry(bestMove, board.GetZobristKey(), score, depthRemaining, board.half_turn_count, distanceFromRoot,
            EntryType::UPPERBOUND); // mate score adjustent is done inside this function
    else if (score >= beta)
        tTable.AddEntry(bestMove, board.GetZobristKey(), score, depthRemaining, board.half_turn_count, distanceFromRoot,
            EntryType::LOWERBOUND);
    else
        tTable.AddEntry(bestMove, board.GetZobristKey(), score, depthRemaining, board.half_turn_count, distanceFromRoot,
            EntryType::EXACT);
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

template <SearchType search_type>
SearchResult Quiescence(GameState& position, SearchStackState* ss, SearchLocalState& local, SearchSharedState& shared,
    int depth, Score alpha, Score beta)
{
    static_assert(search_type != SearchType::ROOT);
    assert((search_type == SearchType::PV) || (beta == alpha + 1));
    constexpr bool pv_node = search_type != SearchType::ZW;
    const auto distance_from_root = ss->distance_from_root;

    // check if we should abort the search
    if (should_abort_search(local, shared))
    {
        return SCORE_UNDEFINED;
    }

    local.sel_septh = std::max(local.sel_septh, distance_from_root);
    local.nodes.fetch_add(1, std::memory_order_relaxed);

    if (distance_from_root >= MAX_DEPTH)
        return 0; // Have we reached max depth?

    if constexpr (pv_node)
    {
        ss->pv.clear();
    }

    if (DeadPosition(position.Board()))
        return 0; // Is this position a dead draw?

    auto staticScore = EvaluatePositionNet(position, local.eval_cache);
    if (staticScore >= beta)
        return staticScore;
    if (staticScore > alpha)
        alpha = staticScore;

    Move bestmove = Move::Uninitialized;
    auto score = staticScore;

    StagedMoveGenerator gen(position, ss, local, Move::Uninitialized, true);
    Move move;

    while (gen.Next(move))
    {
        int SEE = gen.GetSEE(move);

        if (staticScore + SEE + Delta_margin < alpha) // delta pruning
            break;

        if (SEE < 0) // prune bad captures
            break;

        // prune underpromotions
        if (move.IsPromotion() && !(move.GetFlag() == QUEEN_PROMOTION || move.GetFlag() == QUEEN_PROMOTION_CAPTURE))
            break;

        ss->move = move;
        position.ApplyMove(move);
        auto search_score
            = -Quiescence<search_type>(position, ss + 1, local, shared, depth - 1, -beta, -alpha).GetScore();
        position.RevertMove();

        if (local.aborting_search)
        {
            return SCORE_UNDEFINED;
        }

        if (search_score > score)
        {
            bestmove = move;
            score = search_score;

            if (score > alpha)
            {
                alpha = score;

                if constexpr (pv_node)
                {
                    UpdatePV(move, ss);
                }

                if (alpha >= beta)
                {
                    break;
                }
            }
        }
    }

    return SearchResult(score, bestmove);
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
        return;
    gen.AdjustHistory(move, depthRemaining * depthRemaining, -depthRemaining * depthRemaining);
}

bool should_abort_search(SearchLocalState& local, const SearchSharedState& shared)
{
    // If we are currently in the process of aborting, do so as quickly as possible
    if (local.aborting_search)
    {
        return true;
    }

    // See if we should abort the search. We get this signal if all threads have decided they want to stop, or we
    // receive a 'stop' command from the uci input
    if (local.search_depth > 1 && !KeepSearching)
    {
        local.aborting_search = true;
        return true;
    }

    // Check if we have breached the time limit.
    if (local.search_depth > 1 && local.nodes.load(std::memory_order_relaxed) % 1024 == 0
        && shared.limits.HitTimeLimit())
    {
        local.aborting_search = true;
        return true;
    }

    return false;
}