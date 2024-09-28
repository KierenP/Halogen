#include "Evaluate.h"

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Network.h"
#include "Score.h"
#include "SearchData.h"

#include <algorithm>

void TempoAdjustment(Score& eval);

Score Evaluate(const BoardState& board, SearchStackState* ss, Network& net)
{
    // apply lazy updates to accumulator stack
    //
    // we assume the root position always has a valid accumulator, and use pointer arithmatic to get there
    auto* current = ss;
    while (!current->acc.acc_is_valid)
    {
        current--;
    }

    while (current + 1 <= ss)
    {
        net.ApplyLazyUpdates(current->acc, (current + 1)->acc);
        current++;
    }

    assert(Network::Verify(board, ss->acc));
    Score eval = Network::Eval(board, ss->acc);
    return std::clamp<Score>(eval, Score::Limits::EVAL_MIN, Score::Limits::EVAL_MAX);
}

bool DeadPosition(const BoardState& board)
{
    if ((board.GetPieceBB<WHITE_PAWN>()) != 0)
        return false;
    if ((board.GetPieceBB<WHITE_ROOK>()) != 0)
        return false;
    if ((board.GetPieceBB<WHITE_QUEEN>()) != 0)
        return false;

    if ((board.GetPieceBB<BLACK_PAWN>()) != 0)
        return false;
    if ((board.GetPieceBB<BLACK_ROOK>()) != 0)
        return false;
    if ((board.GetPieceBB<BLACK_QUEEN>()) != 0)
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
    int WhiteBishops = GetBitCount(board.GetPieceBB<WHITE_BISHOP>());
    int BlackBishops = GetBitCount(board.GetPieceBB<BLACK_BISHOP>());
    int WhiteKnights = GetBitCount(board.GetPieceBB<WHITE_KNIGHT>());
    int BlackKnights = GetBitCount(board.GetPieceBB<BLACK_KNIGHT>());
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

void TempoAdjustment(Score& eval)
{
    constexpr static int TEMPO = 10;
    eval += TEMPO;
}
