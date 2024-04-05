#include "EvalNet.h"

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "EvalCache.h"
#include "GameState.h"
#include "Score.h"

#include <algorithm>

void TempoAdjustment(Score& eval, const BoardState& board);

Score EvaluatePositionNet(const GameState& position, EvalCacheTable& evalTable)
{
    Score eval = 0;

    if (!evalTable.GetEntry(position.Board().GetZobristKey(), eval))
    {
        eval = position.GetEvaluation();

        TempoAdjustment(eval, position.Board());

        evalTable.AddEntry(position.Board().GetZobristKey(), eval);
    }

    return std::clamp<Score>(eval, Score::Limits::EVAL_MIN, Score::Limits::EVAL_MAX);
}

bool DeadPosition(const BoardState& board)
{
    if ((board.GetPieceBB(WHITE_PAWN)) != 0)
        return false;
    if ((board.GetPieceBB(WHITE_ROOK)) != 0)
        return false;
    if ((board.GetPieceBB(WHITE_QUEEN)) != 0)
        return false;

    if ((board.GetPieceBB(BLACK_PAWN)) != 0)
        return false;
    if ((board.GetPieceBB(BLACK_ROOK)) != 0)
        return false;
    if ((board.GetPieceBB(BLACK_QUEEN)) != 0)
        return false;

    /*
    From the Chess Programming Wiki:
        According to the rules of a dead position, Article 5.2 b, when there is no possibility of checkmate for either
        side with any series of legal moves, the position is an immediate draw if:
            1: both sides have a bare king
            2: one side has a king and a minor piece against a bare king
            Not covered: both sides have a king and a bishop, the bishops being the same color
    */

    // We know the board must contain just knights, bishops and kings
    int WhiteBishops = GetBitCount(board.GetPieceBB(WHITE_BISHOP));
    int BlackBishops = GetBitCount(board.GetPieceBB(BLACK_BISHOP));
    int WhiteKnights = GetBitCount(board.GetPieceBB(WHITE_KNIGHT));
    int BlackKnights = GetBitCount(board.GetPieceBB(BLACK_KNIGHT));
    int WhiteMinor = WhiteBishops + WhiteKnights;
    int BlackMinor = BlackBishops + BlackKnights;

    if (WhiteMinor == 0 && BlackMinor == 0)
        return true; // 1
    if (WhiteMinor == 1 && BlackMinor == 0)
        return true; // 2
    if (WhiteMinor == 0 && BlackMinor == 1)
        return true; // 2

    return false;
}

void TempoAdjustment(Score& eval, const BoardState& board)
{
    constexpr static int TEMPO = 10;
    eval += board.stm == WHITE ? TEMPO : -TEMPO;
}
