#include "encode.h"

training_data convert(BoardState board, int16_t score, float result, Move best_move)
{
    auto us = board.GetPieces<WHITE>();
    auto them = board.GetPieces<BLACK>();

    auto pawn = board.GetPieceBB<PAWN>();
    auto knight = board.GetPieceBB<KNIGHT>();
    auto bishop = board.GetPieceBB<BISHOP>();
    auto rook = board.GetPieceBB<ROOK>();
    auto queen = board.GetPieceBB<QUEEN>();
    auto king = board.GetPieceBB<KING>();

    if (board.stm == BLACK)
    {
        result = 1 - result;
        score = -score;
        best_move = Move(flip_square(best_move.GetFrom()), flip_square(best_move.GetTo()), best_move.GetFlag());
        us = flip_bb(us);
        them = flip_bb(them);
        pawn = flip_bb(pawn);
        knight = flip_bb(knight);
        bishop = flip_bb(bishop);
        rook = flip_bb(rook);
        queen = flip_bb(queen);
        king = flip_bb(king);
        std::swap(us, them);
    }

    training_data data {};
    data.occ = us | them;

    auto idx = 0;
    auto occ = data.occ;
    while (occ)
    {
        auto sq = LSBpop(occ);
        uint8_t piece_type = (pawn & SquareBB[sq]) != 0 ? PAWN
            : (knight & SquareBB[sq]) != 0              ? KNIGHT
            : (bishop & SquareBB[sq]) != 0              ? BISHOP
            : (rook & SquareBB[sq]) != 0                ? ROOK
            : (queen & SquareBB[sq]) != 0               ? QUEEN
            : (king & SquareBB[sq]) != 0                ? KING
                                                        : N_PIECE_TYPES;
        assert(piece_type != N_PIECE_TYPES);
        uint8_t colour = (us & SquareBB[sq]) != 0 ? 0 : (them & SquareBB[sq]) != 0 ? 1 : 2;
        assert(colour != 2);
        auto encoded_piece = ((colour << 3) | piece_type);
        data.pcs[idx / 2] |= encoded_piece << (4 * (idx & 1));
        idx++;
    }

    data.score = score;
    data.result = 2 * result;
    data.ksq = LSB(us & king);
    data.opp_ksq = flip_square(LSB(them & king));
    data.best_move = best_move;

    return data;
}

#include "../GameState.h"

auto flip_test = []()
{
    GameState position;
    position.InitialiseFromFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R2K3R w kq - 0 1");
    auto encoded_white = convert(position.Board(), 100, 1, Move(SQ_E2, SQ_A6, CAPTURE));
    std::array<uint8_t, 32> serialized_white;
    std::memcpy(serialized_white.data(), &encoded_white, sizeof(encoded_white));

    position.InitialiseFromFen("r2k3r/pppbbppp/2n2q1P/1P2p3/3pn3/BN2PNP1/P1PPQPB1/R3K2R b KQ - 0 1");
    auto encoded_black = convert(position.Board(), -100, 0, Move(SQ_E7, SQ_A3, CAPTURE));
    std::array<uint8_t, 32> serialized_black;
    std::memcpy(serialized_black.data(), &encoded_black, sizeof(encoded_black));

    assert(serialized_white == serialized_black);

    return 0;
}();
