#include "evaluation/evaluate.h"

#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "network/network.h"
#include "search/score.h"
#include "spsa/tuneable.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>

Score evaluate(const BoardState& board, NN::Accumulator* acc, NN::Network& net)
{
    // apply lazy updates to accumulator stack
    //
    // we assume the root position always has a valid accumulator, and use pointer arithmatic to get there
    auto* current = acc;
    while (!current->acc_is_valid)
    {
        current--;
    }

    while (current + 1 <= acc)
    {
        net.apply_lazy_updates(*current, *(current + 1));
        current++;
    }

    assert(NN::Network::verify(board, *acc));
    Score eval = NN::Network::eval(board, *acc);

    // Apply material scaling factor
    const auto npMaterial = eval_scale[PAWN] * std::popcount(board.get_pieces_bb(PAWN))
        + eval_scale[KNIGHT] * std::popcount(board.get_pieces_bb(KNIGHT))
        + eval_scale[BISHOP] * std::popcount(board.get_pieces_bb(BISHOP))
        + eval_scale[ROOK] * std::popcount(board.get_pieces_bb(ROOK))
        + eval_scale[QUEEN] * std::popcount(board.get_pieces_bb(QUEEN));
    eval = eval.value() * (eval_scale_const + npMaterial) / 32768;

    return std::clamp<Score>(eval, Score::Limits::EVAL_MIN, Score::Limits::EVAL_MAX);
}

bool insufficient_material(const BoardState& board)
{
    // Any pawn, rook, or queen means material is sufficient
    if (board.get_pieces_bb(PAWN) | board.get_pieces_bb(ROOK) | board.get_pieces_bb(QUEEN))
        return false;

    /*
    From the Chess Programming Wiki:
        According to the rules of a dead position, Article 5.2 b, when there is no possibility of checkmate for either
        side with any series of legal moves, the position is an immediate draw if:
            1: both sides have a bare king
            2: one side has a king and a minor piece against a bare king
            3: both sides have a king and a bishop, the bishops being the same color
    */

    // Only kings, knights, and bishops remain
    const uint64_t all_bishops = board.get_pieces_bb(BISHOP);
    const uint64_t all_knights = board.get_pieces_bb(KNIGHT);
    const int total_minor = std::popcount(all_bishops | all_knights);

    if (total_minor <= 1)
        return true; // 1, 2

    // KBvKB with same-colored bishops is a dead draw
    if (total_minor == 2 && all_knights == 0)
        return (all_bishops & LIGHT_SQUARES) == 0 || (all_bishops & DARK_SQUARES) == 0; // 3

    return false;
}
