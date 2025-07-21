#include "encode.h"

MarlinFormat convert(BoardState board, float result)
{
    const auto white = board.get_pieces_bb(WHITE);
    const auto black = board.get_pieces_bb(BLACK);
    const auto pawn = board.get_pieces_bb(PAWN);
    const auto knight = board.get_pieces_bb(KNIGHT);
    const auto bishop = board.get_pieces_bb(BISHOP);
    const auto rook = board.get_pieces_bb(ROOK);
    const auto queen = board.get_pieces_bb(QUEEN);
    const auto king = board.get_pieces_bb(KING);
    const auto castle_squares = board.castle_squares;

    MarlinFormat format {};
    format.occ = white | black;

    auto idx = 0;
    auto occ = format.occ;
    while (occ)
    {
        auto sq = lsbpop(occ);
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
    format.half_move_clock = board.fifty_move_count;
    format.full_move_counter = (board.half_turn_count + 1) / 2;
    format.score = 0; // unused
    format.result = result * 2; // convert to [0-2] scale
    format.padding = 0; // unused

    return format;
}

uint16_t convert(Move move)
{
    uint16_t packed_move = (move.from() & 0x3F) | ((move.to() & 0x3F) << 6);

    switch (move.flag())
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

int16_t convert(Side stm, Score score)
{
    score = (stm == WHITE ? score : -score); // convert to white-relative score
    return score.value();
}
