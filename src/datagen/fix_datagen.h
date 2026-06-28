#pragma once

#include "datagen/encode.h"
#include "movegen/list.h"
#include "movegen/movegen.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string_view>

void fix_datagen(std::string_view input_path, std::string_view output_path)
{
    /*
    When decoding the viribinpack file, the decoder occasionally runs into illegal moves.
    Upon inspecting the file closely, it was discovered that these illegal moves were
    actually the start of the next game. These corruption points are rare and hard to spot,
    because the binary format of the data makes it interpretable in multiple ways that can
    only be validated by looking at the data in a schematic sense (i.e using chess logic
    to determine how to correctly repair the data).

    This script reads through the file like normal, and validates that each move we decode is
    a valid move on the chess board in that context. If the move is illegal, we instead attempt
    to interpret the illegal move as the start of the next game, decoding the 32 bytes as
    the next MarlinFormat header. If this decoding is successful, this is strong evidence that we
    can simpily insert the game terminator 0x0000 and continue on. If the bytes cannot be interpreted
    as neither a valid move nor a valid game header then the decoding stops and we report an
    unrecoverable error.

    The leading theory is these corruption points originated from truncated shards (likely
    due to worker crashes or AWS spot instances being preempted). When these truncated files
    were concatenated, they form the corruption points we observed.
    */

    std::ifstream input_file(std::string { input_path }, std::ios::binary);
    if (!input_file)
    {
        std::cerr << "Could not open input file\n";
        return;
    }

    std::ofstream output_file(std::string { output_path }, std::ios::binary);
    if (!output_file)
    {
        std::cerr << "Could not open output file\n";
        return;
    }

    auto write_terminator = [&]()
    {
        uint32_t zero = 0;
        output_file.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
    };

    size_t games = 0;
    size_t moves = 0;
    size_t repaired = 0;

    /*
     * Restore a board from a MarlinFormat header.
     *
     * You will need to adapt the BoardState mutation calls here
     * to your actual API.
     */
    auto decode_board = [](const MarlinFormat& format) -> std::optional<BoardState>
    {
        // Validate unused / deterministic fields.
        // These are strong indicators that this is actually a Marlin header.
        if (format.score != 0)
        {
            return std::nullopt;
        }

        if (format.padding != 0)
        {
            return std::nullopt;
        }

        if (format.result > 2)
        {
            return std::nullopt;
        }

        BoardState board {};

        const int piece_count = std::popcount(format.occ);

        if (piece_count < 2 || piece_count > 32)
        {
            return std::nullopt;
        }

        int decoded_pieces = 0;
        int white_kings = 0;
        int black_kings = 0;
        uint64_t occ = format.occ;
        int idx = 0;

        while (occ)
        {
            Square sq = lsbpop(occ);

            uint8_t nibble = (format.pcs[idx / 2] >> ((idx & 1) * 4)) & 0xF;

            idx++;
            decoded_pieces++;

            uint8_t colour = nibble >> 3;
            uint8_t piece = nibble & 0x7;

            if (colour > 1)
            {
                return std::nullopt;
            }

            Side side = colour == 0 ? WHITE : BLACK;

            if (piece == N_PIECE_TYPES)
            {
                // Castle-able rook

                board.add_piece_sq(sq, get_piece(ROOK, side));
                board.castle_squares |= SquareBB[sq];
            }
            else
            {
                if (piece < PAWN || piece > KING)
                {
                    return std::nullopt;
                }

                auto real_piece = get_piece(static_cast<PieceType>(piece), side);

                if (real_piece == WHITE_KING)
                {
                    white_kings++;
                }
                else if (real_piece == BLACK_KING)
                {
                    black_kings++;
                }

                board.add_piece_sq(sq, real_piece);
            }
        }

        if (decoded_pieces != piece_count)
        {
            return std::nullopt;
        }

        if (white_kings != 1 || black_kings != 1)
        {
            return std::nullopt;
        }

        // side to move
        board.stm = (format.stm_ep & 0x80) ? BLACK : WHITE;

        // en passant
        uint8_t ep = format.stm_ep & 0x7F;

        if (ep > 64)
        {
            return std::nullopt;
        }

        if (ep == 64)
        {
            board.en_passant = N_SQUARES;
        }
        else
        {
            board.en_passant = static_cast<Square>(ep);

            auto rank = enum_to<Rank>(board.en_passant);

            if (board.stm == WHITE && rank != RANK_6)
            {
                return std::nullopt;
            }

            if (board.stm == BLACK && rank != RANK_3)
            {
                return std::nullopt;
            }
        }

        // clocks

        if (format.half_move_clock > 100)
        {
            return std::nullopt;
        }

        board.fifty_move_count = format.half_move_clock;
        board.half_turn_count = format.full_move_counter * 2 - (board.stm == WHITE ? 1 : 0);

        return board;
    };

    /*
     * Checks whether the current stream position begins a valid game.
     * Leaves the stream unchanged.
     */
    auto looks_like_header = [&](std::streampos pos) -> bool
    {
        input_file.seekg(pos);
        MarlinFormat header;

        if (!input_file.read(reinterpret_cast<char*>(&header), sizeof(header)))
        {
            input_file.clear();
            input_file.seekg(pos);
            return false;
        }

        auto board = decode_board(header);

        if (!board)
        {
            input_file.clear();
            input_file.seekg(pos);
            return false;
        }

        input_file.clear();
        input_file.seekg(pos);
        return true;
    };

    while (true)
    {
        MarlinFormat header;

        if (!input_file.read(reinterpret_cast<char*>(&header), sizeof(header)))
        {
            break;
        }

        auto board_opt = decode_board(header);

        if (!board_opt)
        {
            std::cerr << "Bad header at " << input_file.tellg() << "\n";
            break;
        }

        BoardState board = *board_opt;
        board.recalculate();
        output_file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        while (true)
        {
            std::streampos move_pos = input_file.tellg();

            uint16_t packed_move;
            int16_t eval;

            if (!input_file.read(reinterpret_cast<char*>(&packed_move), sizeof(packed_move)))
            {
                goto done;
            }

            if (!input_file.read(reinterpret_cast<char*>(&eval), sizeof(eval)))
            {
                goto done;
            }

            if (packed_move == 0 && eval == 0)
            {
                write_terminator();
                break;
            }

            bool found = false;

            BasicMoveList legal_move_list;
            legal_moves(board, legal_move_list);
            for (const Move& move : legal_move_list)
            {
                if (convert(move) == packed_move)
                {
                    board.apply_move(move);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                /*
                 * This is probably a missing terminator.
                 */
                input_file.clear();
                input_file.seekg(move_pos);

                if (looks_like_header(move_pos))
                {
                    write_terminator();
                    repaired++;
                    std::cerr << "Recovered missing terminator at " << move_pos << "\n";
                    break;
                }

                std::cerr << "Unrecoverable corruption at " << move_pos << "\n";
                goto done;
            }

            output_file.write(reinterpret_cast<const char*>(&packed_move), sizeof(packed_move));
            output_file.write(reinterpret_cast<const char*>(&eval), sizeof(eval));
            moves++;
        }
        games++;
    }

done:
    std::cout << "Games: " << games << "\nMoves: " << moves << "\nRepairs: " << repaired << "\n";
}