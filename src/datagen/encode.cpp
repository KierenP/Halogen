#include "encode.h"

training_data convert(BoardState board, int16_t score, float result, Move best_move)
{
    if (board.stm == BLACK)
    {
        result = 1 - result;
    }

    training_data data;
    data.occ = board.GetAllPieces();

    auto idx = 0;
    auto occ = data.occ;
    while (occ)
    {
        auto sq = LSBpop(occ);
        auto piece = board.GetSquare(sq);
        auto piece_type = GetPieceType(piece);
        auto colour = ColourOfPiece(piece) == board.stm;
        auto encoded_piece = ((colour << 3) | piece_type);
        data.pcs[idx / 2] |= encoded_piece << (4 * (idx & 1));
        idx++;
    }

    data.score = score;
    data.result = result;
    data.ksq = board.GetKing(board.stm);
    data.opp_ksq = board.GetKing(!board.stm);
    data.best_move = best_move;

    return data;
}
