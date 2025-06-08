#pragma once

#include "../Move.h"
#include "../MoveGeneration.h"
#include "../MoveList.h"
#include "encode.h"

#include <fstream>
#include <iostream>
#include <string>

/*

All integers in viriformat are little-endian.

The square indexing used represents A1=0, H1=7, A8=56, H8=63.

A viriformat file consists of one or more Games concatenated together.

A Game consists of a marlinformat PackedBoard followed by zero or more Move and Score pairs, terminated by four zero
bytes.

A PackedBoard is a structure of:

A 64-bit occupied-piece bitboard.
A 32-entry array of 4-bit pieces, where the ith entry corresponds to the ith least-significant set bit in the
occupied-piece bitboard. The lower three bits of a piece corresponds to its type: pawn is 0, knight is 1, bishop is 2,
rook is 3, queen is 4, king is 5. Castling rights are represented by setting the piece type of the relevant rook (e.g.
in classical chess, the a1 rook for queenside, or the h1 rook for kingside) to 6 to represent an unmoved rook. A piece
type of 7 is never valid. A piece has its most-significant bit clear if it is a white piece, and set if it is a black
piece. Nonexistent piece entries may be left at zero. An 8-bit side-to-move and en-passant field. The lower seven bits
represent the en-passant target square, or 64 if there is no such square. The most-significant bit is clear if white is
to move, and set if black is to move. An 8-bit halfmove clock. (This field may be left at zero.) A 16-bit fullmove
counter. (This field may be left at zero.) A Score for the position. (This field may be left at zero.) An 8-bit
game-result field; a black win is 0, a draw is 1, a white win is 2. No other values are valid. An unused extra byte. A
Move is a structure packed into a 16-bit integer:

A 6-bit from square.
A 6-bit to square.
Castling moves are represented as king-takes-rook; a classical chess kingside castling would be represented as "E1 to
H1". This is done to support Chess960. A 2-bit promotion piece, where knight is 0, bishop is 1, rook is 2, queen is 3.
If not a promotion move, this field may be left at zero.
A 2-bit move type, where en-passant captures are 1, castling moves are 2, promotions (including capture-promotions)
are 3. Any move not in the stated categories has a move type of 0. A Score is a signed 16-bit integer representing a
white-relative score for said Move.

Example
00000000: ffff 0000 0000 ffff                       ; occupancy: startpos layout
00000008: 6124 5216 0000 0000 8888 8888 e9ac da9e   ; pieces: startpos layout
00000018: 40                                        ; side to move: white
                                                    ; en-passant: none
00000019: 00                                        ; halfmove clock: 0
0000001a: 0100                                      ; fullmove counter: 1
0000001c: 0000                                      ; position score: 0 cp
0000001e: 02                                        ; game result: white win
0000001f: 00                                        ; extra byte
00000020: 0c07 0a00                                 ; 1. e4       +10cp
00000024: 3409 1400                                 ; 1. ... e5   +20cp
00000028: c309 e2ff                                 ; 2. Qh5      -30cp
0000002c: 3c0d ff7f                                 ; 2. ... Ke7  +32767cp
00000030: 2709 ff7f                                 ; 3. Qxe5#    +32767cp
00000034: 0000 0000                                 ; terminator

*/

/*

The format of the input data is 'training_data', which is a structure of:

struct training_data
{
    uint64_t occ;
    std::array<uint8_t, 16> pcs;
    int16_t score;
    int8_t result;
    uint8_t ksq;
    uint8_t opp_ksq;

    uint8_t padding[1];
    Move best_move;
};

note that training_data is relative to the side to move, so this affects the encoding of pcs, score, result, ksq and
opp_ksq.

*/

struct MarlinFormat
{
    uint64_t occ; // 64-bit occupied-piece bitboard
    std::array<uint8_t, 16> pcs; // 32-entry array of 4-bit pieces
    uint8_t stm_ep; // 8-bit side-to-move and en-passant field
    uint8_t half_move_clock; // 8-bit halfmove clock
    uint16_t full_move_counter; // 16-bit fullmove counter
    int16_t score; // Score for the position
    uint8_t result; // Game result: black win is 0, draw is 1, white win is 2
    uint8_t padding; // Unused extra byte
};

static_assert(sizeof(MarlinFormat) == 32, "MarlinFormat size must be 32 bytes");

inline void convert_to_viribinpack(const std::string& input_file, const std::string& output_file)
{
    // In order to convert the file, we assume the input file exists of a series of games, stored as sequential
    // training_data's.

    std::ifstream input(input_file, std::ios::binary);
    if (!input.is_open())
    {
        std::cout << "Could not open input file: " + input_file << std::endl;
        return;
    }

    std::ofstream output(output_file, std::ios::binary);
    if (!output.is_open())
    {
        std::cout << "Could not open output file: " + output_file << std::endl;
        return;
    }

    BoardState prev_board;
    bool first_game = true;

    auto construct_board = [](const training_data& data) -> BoardState
    {
        // setup the position. For simplicity, assume STM is white
        BoardState board;
        board.en_passant = N_SQUARES;
        board.castle_squares = EMPTY;
        board.fifty_move_count = 0;
        board.half_turn_count = 1;
        board.stm = WHITE;
        auto occ = data.occ;
        int piece = 0;
        while (occ)
        {
            auto sq = LSBpop(occ);
            auto encoded_piece = ((data.pcs[piece / 2] >> (4 * (piece % 2))) & 0xF);
            auto piece_type = static_cast<PieceTypes>(encoded_piece & 0x7);
            auto color = !static_cast<Players>(encoded_piece >> 3);
            board.SetSquare(sq, Piece(piece_type, color));
            piece++;
        }
        return board;
    };

    auto has_game_continued = [&](const BoardState& board) -> bool
    {
        // ChessBoard format doesn't have any stm, ep, fifty move info. So all we can do is check if the pieces are on
        // the squares they should be.
        return board.GetPieceBB<WHITE_PAWN>() == prev_board.GetPieceBB<WHITE_PAWN>()
            && board.GetPieceBB<WHITE_KNIGHT>() == prev_board.GetPieceBB<WHITE_KNIGHT>()
            && board.GetPieceBB<WHITE_BISHOP>() == prev_board.GetPieceBB<WHITE_BISHOP>()
            && board.GetPieceBB<WHITE_ROOK>() == prev_board.GetPieceBB<WHITE_ROOK>()
            && board.GetPieceBB<WHITE_QUEEN>() == prev_board.GetPieceBB<WHITE_QUEEN>()
            && board.GetPieceBB<WHITE_KING>() == prev_board.GetPieceBB<WHITE_KING>()
            && board.GetPieceBB<BLACK_PAWN>() == prev_board.GetPieceBB<BLACK_PAWN>()
            && board.GetPieceBB<BLACK_KNIGHT>() == prev_board.GetPieceBB<BLACK_KNIGHT>()
            && board.GetPieceBB<BLACK_BISHOP>() == prev_board.GetPieceBB<BLACK_BISHOP>()
            && board.GetPieceBB<BLACK_ROOK>() == prev_board.GetPieceBB<BLACK_ROOK>()
            && board.GetPieceBB<BLACK_QUEEN>() == prev_board.GetPieceBB<BLACK_QUEEN>()
            && board.GetPieceBB<BLACK_KING>() == prev_board.GetPieceBB<BLACK_KING>();
    };

    auto write_marlin_format = [&](const BoardState& board, uint8_t result)
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

        MarlinFormat format;
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
        format.result = board.stm == WHITE ? result : 2 - result; // convert to white-relative result
        format.padding = 0; // unused

        output.write(reinterpret_cast<const char*>(&format), sizeof(format));
    };

    auto write_viri_move_score = [&](Move move, int16_t score, Players stm)
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

        score = (stm == WHITE ? score : -score); // convert to white-relative score
        output.write(reinterpret_cast<const char*>(&packed_move), sizeof(packed_move));
        output.write(reinterpret_cast<const char*>(&score), sizeof(score));
    };

    int64_t number_of_games = 0;
    int64_t number_of_training_data = 0;

    training_data data;
    while (input.read((char*)(&data), sizeof(training_data)))
    {
        auto curr_board = construct_board(data);

        if (has_game_continued(curr_board))
        {
            // If the game has continued, we just write the next move and score
            write_viri_move_score(data.best_move, data.score, curr_board.stm);
            number_of_training_data++;

            prev_board.ApplyMove(data.best_move);
        }
        else
        {
            if (first_game)
            {
                first_game = false;
            }
            else
            {
                // Write the terminator for the previous game
                uint8_t terminator[4] = { 0, 0, 0, 0 };
                output.write(reinterpret_cast<const char*>(terminator), sizeof(terminator));
            }

            // Write the MarlinFormat for the new game
            write_marlin_format(curr_board, data.result);
            number_of_games++;

            // Write the first move and score for the new game
            write_viri_move_score(data.best_move, data.score, curr_board.stm);
            number_of_training_data++;

            prev_board = curr_board;
            prev_board.ApplyMove(data.best_move);
        }
    }

    std::cout << "Converted " << number_of_games << " games with " << number_of_training_data
              << " training data points." << std::endl;
}