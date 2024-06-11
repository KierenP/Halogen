#pragma once

#include "../EGTB.h"
#include "../Pyrrhic/tbprobe.h"
#include "encode.h"

#include <cmath>
#include <fstream>
#include <iostream>

inline void filter(std::string_view input_path, std::string_view output_path)
{
    /*std::ifstream input(input_path.data(), std::ios::binary);
    std::ofstream output(output_path.data(), std::ios::binary);

    training_data data;
    size_t datapoint_count = 0;
    size_t filtered_count = 0;

    auto print_diagnostics = [&]()
    {
        std::cout << "Read " << datapoint_count << " points. Positions: " << datapoint_count
                  << " Filtered: " << filtered_count << std::endl;
    };

    while (input.read((char*)(&data), sizeof(training_data)))
    {
        datapoint_count++;

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

        if (!data.best_move.IsCapture() && !data.best_move.IsPromotion() && !IsInCheck(board))
        {
            filtered_count++;
            output.write((char*)(&data), sizeof(training_data));
        }

        if (datapoint_count % (1024 * 1024) == 0)
        {
            print_diagnostics();
        }
    }

    print_diagnostics();
    std::cout << "Complete" << std::endl;*/
}