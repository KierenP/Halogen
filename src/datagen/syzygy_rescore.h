#pragma once

#include "../EGTB.h"
#include "../Pyrrhic/tbprobe.h"
#include "encode.h"

#include <cmath>
#include <fstream>
#include <iostream>

inline float sigmoid(float x)
{
    return 1.0 / (1.0 + std::exp(-x / 160.0));
}

inline void syzygy_rescore(std::string_view input_path, std::string_view output_path)
{
    std::ifstream input(input_path.data(), std::ios::binary);
    std::ofstream output(output_path.data(), std::ios::binary);

    training_data data;
    size_t datapoint_count = 0;
    size_t syzygy_count = 0;
    size_t correct_wdl = 0;
    double mse_eval = 0;

    auto print_diagnostics = [&]()
    {
        std::cout << "Read " << datapoint_count << " points. Syzygy positions: " << syzygy_count
                  << " WDL correctness: " << double(correct_wdl) / double(syzygy_count)
                  << " Eval MSE: " << mse_eval / syzygy_count << std::endl;
    };

    while (input.read((char*)(&data), sizeof(training_data)))
    {
        datapoint_count++;

        if (GetBitCount(data.occ) <= TB_LARGEST)
        {
            syzygy_count++;

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

            auto probe = Syzygy::probe_wdl_search(board, 0);

            if (!probe)
            {
                std::cout << "Failed Syzygy probe on position: " << std::endl;
                std::cout << board << std::endl;
                continue;
            }

            auto probe_result = probe.value() == Score::tb_win_in(0) ? 2 : probe.value() == Score::draw() ? 1 : 0;

            if (data.result == probe_result)
            {
                correct_wdl++;
            }

            auto eval = sigmoid(data.score);
            auto correct = double(probe_result) / 2;
            mse_eval += (eval - correct) * (eval - correct);

            // set the result and score fields based on the correct true syzygy result
            data.result = probe_result;
            data.score = probe->value();

            output.write((char*)(&data), sizeof(training_data));
        }
        else
        {
            output.write((char*)(&data), sizeof(training_data));
        }

        if (datapoint_count % (1024 * 1024) == 0)
        {
            print_diagnostics();
        }
    }

    print_diagnostics();
    std::cout << "Complete" << std::endl;
}