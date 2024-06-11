#include "encode.h"

MarlinFormat convert(BoardState board, float result)
{
    const auto white = board.GetPieces<WHITE>();
    const auto black = board.GetPieces<BLACK>();
    const auto pawn = board.GetPieceBB<PAWN>();
    const auto knight = board.GetPieceBB<KNIGHT>();
    const auto bishop = board.GetPieceBB<BISHOP>();
    const auto rook = board.GetPieceBB<ROOK>();
    const auto queen = board.GetPieceBB<QUEEN>();
    const auto king = board.GetPieceBB<KING>();
    const auto castle_squares = board.castle_squares;

    MarlinFormat format {};
    format.occ = white | black;

    auto idx = 0;
    auto occ = format.occ;
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
        if (castle_squares & SquareBB[sq])
        {
            // marlin format encodes castle-able rooks as piece type 6
            assert(piece_type == ROOK);
            piece_type = N_PIECE_TYPES;
        }
        uint8_t colour = (white & SquareBB[sq]) != 0 ? 0 : (black & SquareBB[sq]) != 0 ? 1 : 2;
        assert(colour != 2);
        auto encoded_piece = ((colour << 3) | piece_type);
        format.pcs[idx / 2] |= encoded_piece << (4 * (idx & 1));
        idx++;
    }

    format.stm_ep = (board.stm == WHITE ? 0 : 0x80) | (board.en_passant == N_SQUARES ? 64 : board.en_passant);
    format.half_move_clock = 0; // unused
    format.full_move_counter = 0; // unused
    format.score = 0; // unused
    format.result = result * 2; // convert to [0-2] scale
    format.padding = 0; // unused

    return format;
}

uint16_t convert(Move move)
{
    uint16_t packed_move = (move.GetFrom() & 0x3F) | ((move.GetTo() & 0x3F) << 6);

    switch (move.GetFlag())
    {
    case KNIGHT_PROMOTION:
    case KNIGHT_PROMOTION_CAPTURE:
        packed_move |= 0b1100 << 12; // knight promotion
        break;
    case BISHOP_PROMOTION:
    case BISHOP_PROMOTION_CAPTURE:
        packed_move |= 0b1101 << 12; // bishop promotion
        break;
    case ROOK_PROMOTION:
    case ROOK_PROMOTION_CAPTURE:
        packed_move |= 0b1110 << 12; // rook promotion
        break;
    case QUEEN_PROMOTION:
    case QUEEN_PROMOTION_CAPTURE:
        packed_move |= 0b1111 << 12; // queen promotion
        break;
    case EN_PASSANT:
        packed_move |= 0b0100 << 12; // en-passant capture
        break;
    case A_SIDE_CASTLE:
    case H_SIDE_CASTLE:
        packed_move |= 0b1000 << 12; // castling move
        break;
    default:
        break;
    }

    return packed_move;
}

int16_t convert(Players stm, Score score)
{
    score = (stm == WHITE ? score : -score); // convert to white-relative score
    return score.value();
}
