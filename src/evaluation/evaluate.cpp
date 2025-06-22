#include "evaluation/evaluate.h"

#include <algorithm>
#include <cassert>

#include "Score.h"
#include "SearchData.h"
#include "bitboard.h"
#include "chessboard/board_state.h"
#include "network/network.h"

Score evaluate(const BoardState& board, SearchStackState* ss, NN::Network& net)
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
        net.apply_lazy_updates(current->acc, (current + 1)->acc);
        current++;
    }

    assert(NN::Network::verify(board, ss->acc));
    Score eval = NN::Network::eval(board, ss->acc);

    // Apply material scaling factor
    const auto npMaterial = 450 * popcount(board.get_pieces_bb<KNIGHT>())
        + 450 * popcount(board.get_pieces_bb<BISHOP>()) + 650 * popcount(board.get_pieces_bb<ROOK>())
        + 1250 * popcount(board.get_pieces_bb<QUEEN>());
    eval = eval.value() * (26500 + npMaterial) / 32768;

    return std::clamp<Score>(eval, Score::Limits::EVAL_MIN, Score::Limits::EVAL_MAX);
}

bool insufficient_material(const BoardState& board)
{
    if ((board.get_pieces_bb<WHITE_PAWN>()) != 0)
        return false;
    if ((board.get_pieces_bb<WHITE_ROOK>()) != 0)
        return false;
    if ((board.get_pieces_bb<WHITE_QUEEN>()) != 0)
        return false;

    if ((board.get_pieces_bb<BLACK_PAWN>()) != 0)
        return false;
    if ((board.get_pieces_bb<BLACK_ROOK>()) != 0)
        return false;
    if ((board.get_pieces_bb<BLACK_QUEEN>()) != 0)
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
    int WhiteBishops = popcount(board.get_pieces_bb<WHITE_BISHOP>());
    int BlackBishops = popcount(board.get_pieces_bb<BLACK_BISHOP>());
    int WhiteKnights = popcount(board.get_pieces_bb<WHITE_KNIGHT>());
    int BlackKnights = popcount(board.get_pieces_bb<BLACK_KNIGHT>());
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
