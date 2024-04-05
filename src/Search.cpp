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
#include "EvalNet.h"
#include "GameState.h"
#include "MoveGeneration.h"
#include "MoveList.h"
#include "Pyrrhic/tbprobe.h"
#include "Score.h"
#include "SearchData.h"
#include "StagedMoveGenerator.h"
#include "TTEntry.h"
#include "TimeManage.h"
#include "TranspositionTable.h"
#include "Zobrist.h"

// [depth][move number]
const std::array<std::array<int, 64>, 64> LMR_reduction = []
{
    std::array<std::array<int, 64>, 64> ret = {};

    for (size_t i = 0; i < ret.size(); i++)
    {
        for (size_t j = 0; j < ret[i].size(); j++)
        {
            ret[i][j] = static_cast<int>(std::round(LMR_constant + LMR_coeff * log(i + 1) * log(j + 1)));
        }
    }

    return ret;
}();

void PrintBestMove(Move Best, const BoardState& board, bool chess960);
bool UseTransposition(const TTEntry& entry, Score alpha, Score beta);
bool CheckForRep(const GameState& position, int distanceFromRoot);
bool AllowedNull(bool allowedNull, const BoardState& board, Score beta, Score alpha, bool InCheck);
bool IsEndGame(const BoardState& board);
bool IsPV(Score beta, Score alpha);
void AddScoreToTable(Score score, Score alphaOriginal, const BoardState& board, int depthRemaining,
    int distanceFromRoot, Score beta, Move bestMove);
void UpdateBounds(const TTEntry& entry, Score& alpha, Score& beta);
Score TerminalScore(const BoardState& board, int distanceFromRoot);
int extension(const BoardState& board);
void AddKiller(Move move, int distanceFromRoot, std::array<std::array<Move, 2>, MAX_DEPTH>& KillerMoves);
void AddHistory(const StagedMoveGenerator& gen, const Move& move, SearchData& locals, int depthRemaining);
void UpdatePV(Move move, int distanceFromRoot, std::array<BasicMoveList, MAX_DEPTH>& PvTable);
int Reduction(int depth, int i);
unsigned int ProbeTBRoot(const BoardState& board);
unsigned int ProbeTBSearch(const BoardState& board);
SearchResult UseSearchTBScore(unsigned int result, int distanceFromRoot);
Move GetTBMove(unsigned int result);

void SearchPosition(GameState position, ThreadSharedData& sharedData, unsigned int threadID);
SearchResult AspirationWindowSearch(
    GameState position, int depth, Score prevScore, SearchData& locals, ThreadSharedData& sharedData);
SearchResult NegaScout(GameState& position, unsigned int initialDepth, int depthRemaining, Score alpha, Score beta,
    int colour, unsigned int distanceFromRoot, bool allowedNull, SearchData& locals, ThreadSharedData& sharedData);
void UpdateAlpha(Score score, Score& a, const Move& move, unsigned int distanceFromRoot, SearchData& locals);
void UpdateScore(Score newScore, Score& score, Move& bestMove, const Move& move);
SearchResult Quiescence(GameState& position, unsigned int initialDepth, Score alpha, Score beta, int colour,
    unsigned int distanceFromRoot, int depthRemaining, SearchData& locals, ThreadSharedData& sharedData);

bool should_abort_search(int initial_depth, SearchData& locals, const ThreadSharedData& shared_data);

uint64_t SearchThread(GameState position, ThreadSharedData& sharedData)
{
    // Probe TB at root
    if (GetBitCount(position.Board().GetAllPieces()) <= TB_LARGEST && position.Board().castle_squares == EMPTY)
    {
        unsigned int result = ProbeTBRoot(position.Board());
        if (result != TB_RESULT_FAILED)
        {
            PrintBestMove(GetTBMove(result), position.Board(), sharedData.GetParameters().chess960);
            return 0;
        }
    }

    sharedData.ResetNewSearch();

    // Limit the MultiPV setting to be at most the number of legal moves
    BasicMoveList moves;
    moves.clear();
    LegalMoves(position.Board(), moves);
    sharedData.SetMultiPv(std::min<int>(sharedData.GetParameters().multiPV, moves.size()));

    KeepSearching = true;

    std::vector<std::thread> threads;

    for (int i = 0; i < sharedData.GetParameters().threads; i++)
    {
        threads.emplace_back(std::thread([=, &sharedData] { SearchPosition(position, sharedData, i); }));
    }

    for (size_t i = 0; i < threads.size(); i++)
    {
        threads[i].join();
    }

    PrintBestMove(sharedData.GetBestMove(), position.Board(), sharedData.GetParameters().chess960);
    return sharedData.getNodes(); // Used by bench searches. Otherwise is discarded.
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

void SearchPosition(GameState position, ThreadSharedData& sharedData, unsigned int threadID)
{
    auto alpha = std::numeric_limits<Score>::min();
    auto beta = std::numeric_limits<Score>::max();
    Score prevScore = 0;

    while (sharedData.GetDepth() < MAX_DEPTH)
    {
        int depth = sharedData.GetDepth();

        if (!KeepSearching)
            break;
        if (sharedData.GetLimits().HitDepthLimit(depth))
            break;
        if (sharedData.GetLimits().HitNodeLimit(sharedData.getNodes()))
            break;
        if (!sharedData.GetLimits().ShouldContinueSearch())
            sharedData.ReportWantsToStop(threadID);

        SearchResult curSearch
            = AspirationWindowSearch(position, depth, prevScore, sharedData.GetData(threadID), sharedData);

        // If we aborted because another thread finished the depth we were on, get that score and continue to that
        // depth.
        if (sharedData.ThreadAbort(depth))
        {
            prevScore = sharedData.GetAspirationScore();
            sharedData.GetData(threadID).aborting_search = false;
            continue;
        }

        // Else, if we aborted then we should return
        if (sharedData.GetData(threadID).aborting_search)
        {
            return;
        }

        sharedData.ReportResult(depth, sharedData.GetLimits().ElapsedTime(), curSearch.GetScore(), alpha, beta,
            position, curSearch.GetMove(), sharedData.GetData(threadID), sharedData.GetParameters().chess960);
        if (sharedData.GetLimits().HitMateLimit(curSearch.GetScore()))
            break;
        prevScore = curSearch.GetScore();
    }
}

SearchResult AspirationWindowSearch(
    GameState position, int depth, Score prevScore, SearchData& locals, ThreadSharedData& sharedData)
{
    int delta = Aspiration_window;

    Score alpha = prevScore.value() - delta;
    Score beta = prevScore.value() + delta;
    SearchResult search = { 0 };

    while (true)
    {
        locals.ResetSeldepth();
        search = NegaScout(
            position, depth, depth, alpha, beta, position.Board().stm ? 1 : -1, 0, false, locals, sharedData);

        if (locals.aborting_search)
        {
            return SCORE_UNDEFINED;
        }

        if (alpha < search.GetScore() && search.GetScore() < beta)
            break;
        if (sharedData.ThreadAbort(depth))
            break;
        if (depth > 1 && sharedData.GetLimits().HitTimeLimit())
            break;

        if (search.GetScore() <= alpha)
        {
            sharedData.ReportResult(depth, sharedData.GetLimits().ElapsedTime(), alpha, alpha, beta, position,
                search.GetMove(), locals, sharedData.GetParameters().chess960);
            beta = (alpha + beta) / 2;
            alpha = std::max<Score>(Score::Limits::MATED, alpha - delta);
        }

        if (search.GetScore() >= beta)
        {
            sharedData.ReportResult(depth, sharedData.GetLimits().ElapsedTime(), beta, alpha, beta, position,
                search.GetMove(), locals, sharedData.GetParameters().chess960);
            sharedData.UpdateBestMove(search.GetMove());
            beta = std::min<Score>(Score::Limits::MATE, beta + delta);
        }

        delta = delta + delta / 2;
    }

    return search;
}

SearchResult NegaScout(GameState& position, unsigned int initialDepth, int depthRemaining, Score alpha, Score beta,
    int colour, unsigned int distanceFromRoot, bool allowedNull, SearchData& locals, ThreadSharedData& sharedData)
{
    // check if we should abort the search
    if (should_abort_search(initialDepth, locals, sharedData))
    {
        return SCORE_UNDEFINED;
    }

    locals.ReportDepth(distanceFromRoot);
    locals.AddNode();

    if (distanceFromRoot >= MAX_DEPTH)
        return 0; // Have we reached max depth?
    locals.PvTable[distanceFromRoot].clear();

    if (DeadPosition(position.Board()))
        return 0; // Is this position a dead draw?
    if (CheckForRep(position, distanceFromRoot) // Have we had a draw by repitition?
        || position.Board().fifty_move_count
            > 100) // cannot use >= as it could currently be checkmate which would count as a win
        return 8
            - (locals.GetThreadNodes()
                & 0b1111); // as in https://github.com/Luecx/Koivisto/commit/c8f01211c290a582b69e4299400b667a7731a9f7
                           // with permission from Koivisto authors.

    auto score = std::numeric_limits<Score>::min();
    auto MaxScore = std::numeric_limits<Score>::max();

    // Probe TB in search
    if (position.Board().fifty_move_count == 0 && GetBitCount(position.Board().GetAllPieces()) <= TB_LARGEST
        && position.Board().castle_squares == EMPTY)
    {
        unsigned int result = ProbeTBSearch(position.Board());
        if (result != TB_RESULT_FAILED)
        {
            locals.AddTbHit();
            auto probe = UseSearchTBScore(result, distanceFromRoot);

            // TODO: check for boundary conditions
            if (probe.GetScore() == 0)
                return probe;
            if (probe.GetScore() >= Score::tb_win_in(MAX_DEPTH) && probe.GetScore() >= beta)
                return probe;
            if (probe.GetScore() <= Score::tb_loss_in(MAX_DEPTH) && probe.GetScore() <= alpha)
                return probe;

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

            if (IsPV(beta, alpha))
            {
                if (probe.GetScore() >= Score::tb_win_in(MAX_DEPTH))
                {
                    score = probe.GetScore();
                    alpha = std::max(alpha, probe.GetScore());
                }
                else
                {
                    MaxScore = probe.GetScore();
                }
            }
        }
    }

    // Query the transpotition table
    if (!IsPV(beta, alpha))
    {
        auto entry = tTable.GetEntryMinDepth(
            position.Board().GetZobristKey(), depthRemaining, distanceFromRoot, position.Board().half_turn_count);
        if (entry.has_value())
        {
            // Don't take scores from the TT if there's a two-fold repitition
            if (!position.CheckForRep(distanceFromRoot, 2) && UseTransposition(entry.value(), alpha, beta))
                return SearchResult(entry->GetScore(), entry->GetMove());
        }
    }

    bool InCheck = IsInCheck(position.Board());

    // Drop into quiescence search
    if (depthRemaining <= 0 && !InCheck)
    {
        return Quiescence(
            position, initialDepth, alpha, beta, colour, distanceFromRoot, depthRemaining, locals, sharedData);
    }

    auto staticScore = EvaluatePositionNet(position, locals.evalTable) * colour;

    // Static null move pruning
    if (depthRemaining < SNMP_depth && staticScore - SNMP_coeff * depthRemaining >= beta && !InCheck
        && !IsPV(beta, alpha))
        return beta;

    // Null move pruning
    if (AllowedNull(allowedNull, position.Board(), beta, alpha, InCheck) && (staticScore > beta))
    {
        unsigned int reduction = Null_constant + depthRemaining / Null_depth_quotent
            + std::min(3, (staticScore.value() - beta.value()) / Null_beta_quotent);

        position.ApplyNullMove();
        auto null_move_score = -NegaScout(position, initialDepth, depthRemaining - reduction - 1, -beta, -beta + 1,
            -colour, distanceFromRoot + 1, false, locals, sharedData)
                                    .GetScore();
        position.RevertNullMove();

        if (null_move_score >= beta)
        {
            if (beta < Score::mated_in(MAX_DEPTH)
                || depthRemaining >= 10) // TODO: I'm not sure about this first condition
            {
                // Do verification search for high depths
                SearchResult result = NegaScout(position, initialDepth, depthRemaining - reduction - 1, beta - 1, beta,
                    colour, distanceFromRoot, false, locals, sharedData);
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
    alpha = std::max(Score::mated_in(distanceFromRoot), alpha);
    beta = std::min(Score::mate_in(distanceFromRoot), beta);
    if (alpha >= beta)
        return alpha;

    // Set up search variables
    Move bestMove = Move::Uninitialized;
    auto a = alpha;
    auto b = beta;
    int searchedMoves;
    bool noLegalMoves = true;

    // Rebel style IID. Don't ask why this helps but it does.
    if (GetHashMove(position.Board(), distanceFromRoot) == Move::Uninitialized && depthRemaining > 3)
        depthRemaining--;

    bool FutileNode
        = depthRemaining < Futility_depth && staticScore + Futility_constant + Futility_coeff * depthRemaining < a;

    StagedMoveGenerator gen(position, distanceFromRoot, locals, false);
    Move move;

    for (searchedMoves = 0; gen.Next(move); searchedMoves++)
    {
        if (distanceFromRoot == 0 && sharedData.MultiPVExcludeMove(move))
            continue;

        noLegalMoves = false;

        // late move pruning
        if (depthRemaining < LMP_depth && searchedMoves >= LMP_constant + LMP_coeff * depthRemaining
            && score > Score::tb_loss_in(MAX_DEPTH))
            gen.SkipQuiets();

        // futility pruning
        if (searchedMoves > 0 && FutileNode && !IsPV(beta, alpha) && !InCheck && score > Score::tb_loss_in(MAX_DEPTH))
        {
            gen.SkipQuiets();
            if (gen.GetStage() >= Stage::GIVE_BAD_LOUD)
                break;
        }

        int history = locals.history.Get(position, move);

        position.ApplyMove(move);
        tTable.PreFetch(position.Board().GetZobristKey()); // load the transposition into l1 cache. ~5% speedup
        int extendedDepth = depthRemaining + extension(position.Board());

        // late move reductions
        if (searchedMoves > 3)
        {
            int reduction = Reduction(depthRemaining, searchedMoves);

            if (IsPV(beta, alpha))
                reduction--;

            reduction -= history / 8192;

            reduction = std::max(0, reduction);

            auto late_move_score = -NegaScout(position, initialDepth, extendedDepth - 1 - reduction, -a - 1, -a,
                -colour, distanceFromRoot + 1, true, locals, sharedData)
                                        .GetScore();

            if (late_move_score <= a)
            {
                position.RevertMove();
                continue;
            }
        }

        auto newScore = -NegaScout(
            position, initialDepth, extendedDepth - 1, -b, -a, -colour, distanceFromRoot + 1, true, locals, sharedData)
                             .GetScore();
        if (newScore > a && newScore < beta && searchedMoves >= 1) // MultiPV issues here
        {
            newScore = -NegaScout(position, initialDepth, extendedDepth - 1, -beta, -a, -colour, distanceFromRoot + 1,
                true, locals, sharedData)
                            .GetScore();
        }

        position.RevertMove();

        UpdateScore(newScore, score, bestMove, move);
        UpdateAlpha(score, a, move, distanceFromRoot, locals);

        // avoid updating Killers or History when aborting the search
        // check for fail high cutoff
        if (!locals.aborting_search && a >= beta)
        {
            AddKiller(move, distanceFromRoot, locals.KillerMoves);
            AddHistory(gen, move, locals, depthRemaining);
            break;
        }

        b = a + 1; // Set a new zero width window
    }

    // Checkmate or stalemate
    if (noLegalMoves)
    {
        return TerminalScore(position.Board(), distanceFromRoot);
    }

    score = std::min(score, MaxScore);

    // avoid adding scores to the TT when we are aborting the search
    if (!locals.aborting_search)
    {
        AddScoreToTable(score, alpha, position.Board(), depthRemaining, distanceFromRoot, beta, bestMove);
    }

    return SearchResult(score, bestMove);
}

unsigned int ProbeTBRoot(const BoardState& board)
{
    // clang-format off
    return tb_probe_root(board.GetWhitePieces(), board.GetBlackPieces(),
        board.GetPieceBB<KING>(),
        board.GetPieceBB<QUEEN>(),
        board.GetPieceBB<ROOK>(),
        board.GetPieceBB<BISHOP>(),
        board.GetPieceBB<KNIGHT>(),
        board.GetPieceBB<PAWN>(),
        board.fifty_move_count,
        board.en_passant <= SQ_H8 ? board.en_passant : 0,
        board.stm == WHITE,
        NULL);
    // clang-format on
}

unsigned int ProbeTBSearch(const BoardState& board)
{
    // clang-format off
    return tb_probe_wdl(board.GetWhitePieces(), board.GetBlackPieces(),
        board.GetPieceBB<KING>(),
        board.GetPieceBB<QUEEN>(),
        board.GetPieceBB<ROOK>(),
        board.GetPieceBB<BISHOP>(),
        board.GetPieceBB<KNIGHT>(),
        board.GetPieceBB<PAWN>(),
        board.en_passant <= SQ_H8 ? board.en_passant : 0,
        board.stm == WHITE);
    // clang-format on
}

SearchResult UseSearchTBScore(unsigned int result, int distanceFromRoot)
{
    if (result == TB_LOSS)
        return Score::tb_loss_in(distanceFromRoot);
    else if (result == TB_BLESSED_LOSS)
        return 0;
    else if (result == TB_DRAW)
        return 0;
    else if (result == TB_CURSED_WIN)
        return 0;
    else if (result == TB_WIN)
        return Score::tb_win_in(distanceFromRoot);
    else
        assert(0);
}

Move GetTBMove(unsigned int result)
{
    int flag = -1;

    if (TB_GET_PROMOTES(result) == TB_PROMOTES_NONE)
        flag = QUIET;
    else if (TB_GET_PROMOTES(result) == TB_PROMOTES_KNIGHT)
        flag = KNIGHT_PROMOTION;
    else if (TB_GET_PROMOTES(result) == TB_PROMOTES_BISHOP)
        flag = BISHOP_PROMOTION;
    else if (TB_GET_PROMOTES(result) == TB_PROMOTES_ROOK)
        flag = ROOK_PROMOTION;
    else if (TB_GET_PROMOTES(result) == TB_PROMOTES_QUEEN)
        flag = QUEEN_PROMOTION;
    else
        assert(0);

    return Move(
        static_cast<Square>(TB_GET_FROM(result)), static_cast<Square>(TB_GET_TO(result)), static_cast<MoveFlag>(flag));
}

void UpdateAlpha(Score score, Score& a, const Move& move, unsigned int distanceFromRoot, SearchData& locals)
{
    if (score > a)
    {
        a = score;
        UpdatePV(move, distanceFromRoot, locals.PvTable);
    }
}

void UpdateScore(Score newScore, Score& Score, Move& bestMove, const Move& move)
{
    if (newScore > Score)
    {
        Score = newScore;
        bestMove = move;
    }
}

int Reduction(int depth, int i)
{
    return LMR_reduction[std::min(63, std::max(0, depth))][std::min(63, std::max(0, i))];
}

void UpdatePV(Move move, int distanceFromRoot, std::array<BasicMoveList, MAX_DEPTH>& PvTable)
{
    PvTable[distanceFromRoot].clear();
    PvTable[distanceFromRoot].emplace_back(move);

    if (distanceFromRoot + 1 < static_cast<int>(PvTable.size()))
        PvTable[distanceFromRoot].append(PvTable[distanceFromRoot + 1].begin(), PvTable[distanceFromRoot + 1].end());
}

bool UseTransposition(const TTEntry& entry, Score alpha, Score beta)
{
    if (entry.GetCutoff() == EntryType::EXACT)
        return true;

    auto NewAlpha = alpha;
    auto NewBeta = beta;

    // aspiration windows and search instability lead to issues with shrinking the original window
    UpdateBounds(entry, NewAlpha, NewBeta);

    if (NewAlpha >= NewBeta)
        return true;

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

bool AllowedNull(bool allowedNull, const BoardState& board, Score beta, Score alpha, bool InCheck)
{
    // avoid null move pruning in very late game positions due to zanauag issues.
    // Even with verification search e.g 8/6k1/8/8/8/8/1K6/Q7 w - - 0 1
    return allowedNull && !InCheck && !IsPV(beta, alpha) && !IsEndGame(board) && GetBitCount(board.GetAllPieces()) >= 5;
}

bool IsEndGame(const BoardState& board)
{
    return (
        board.GetPiecesColour(board.stm) == (board.GetPieceBB(KING, board.stm) | board.GetPieceBB(PAWN, board.stm)));
}

bool IsPV(Score beta, Score alpha)
{
    return beta != alpha + 1;
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

void UpdateBounds(const TTEntry& entry, Score& alpha, Score& beta)
{
    if (entry.GetCutoff() == EntryType::LOWERBOUND)
    {
        alpha = std::max(alpha, entry.GetScore());
    }

    if (entry.GetCutoff() == EntryType::UPPERBOUND)
    {
        beta = std::min(beta, entry.GetScore());
    }
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

SearchResult Quiescence(GameState& position, unsigned int initialDepth, Score alpha, Score beta, int colour,
    unsigned int distanceFromRoot, int depthRemaining, SearchData& locals, ThreadSharedData& sharedData)
{
    // check if we should abort the search
    if (should_abort_search(initialDepth, locals, sharedData))
    {
        return SCORE_UNDEFINED;
    }

    locals.ReportDepth(distanceFromRoot);
    locals.AddNode();

    if (distanceFromRoot >= MAX_DEPTH)
        return 0; // Have we reached max depth?
    locals.PvTable[distanceFromRoot].clear();

    if (DeadPosition(position.Board()))
        return 0; // Is this position a dead draw?

    auto staticScore = EvaluatePositionNet(position, locals.evalTable) * colour;
    if (staticScore >= beta)
        return staticScore;
    if (staticScore > alpha)
        alpha = staticScore;

    Move bestmove = Move::Uninitialized;
    auto Score = staticScore;

    StagedMoveGenerator gen(position, distanceFromRoot, locals, true);
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

        position.ApplyMove(move);
        auto newScore = -Quiescence(position, initialDepth, -beta, -alpha, -colour, distanceFromRoot + 1,
            depthRemaining - 1, locals, sharedData)
                             .GetScore();
        position.RevertMove();

        UpdateScore(newScore, Score, bestmove, move);
        UpdateAlpha(Score, alpha, move, distanceFromRoot, locals);

        if (Score >= beta)
            break;
    }

    return SearchResult(Score, bestmove);
}

void AddKiller(Move move, int distanceFromRoot, std::array<std::array<Move, 2>, MAX_DEPTH>& KillerMoves)
{
    if (move.IsCapture() || move.IsPromotion() || KillerMoves[distanceFromRoot][0] == move)
        return;

    KillerMoves[distanceFromRoot][1] = KillerMoves[distanceFromRoot][0];
    KillerMoves[distanceFromRoot][0] = move;
}

void AddHistory(const StagedMoveGenerator& gen, const Move& move, SearchData& locals, int depthRemaining)
{
    if (depthRemaining > 20 || move.IsCapture() || move.IsPromotion())
        return;
    gen.AdjustHistory(move, locals, depthRemaining);
}

bool should_abort_search(int initial_depth, SearchData& locals, const ThreadSharedData& shared_data)
{
    // If we are currently in the process of aborting, do so as quickly as possible
    if (locals.aborting_search)
    {
        return true;
    }

    // See if we should abort the search. We get this signal if all threads have decided they want to stop, or we
    // receive a 'stop' command from the uci input
    if (initial_depth > 1 && !KeepSearching)
    {
        locals.aborting_search = true;
        return true;
    }

    // Check if we have breached the time limit.
    if (initial_depth > 1 && locals.GetThreadNodes() % 1024 == 0 && shared_data.GetLimits().HitTimeLimit())
    {
        locals.aborting_search = true;
        return true;
    }

    // If the current depth has been completed by another thread, we abort and resume at the higher depth
    if (shared_data.ThreadAbort(initial_depth))
    {
        locals.aborting_search = true;
        return true;
    }

    return false;
}