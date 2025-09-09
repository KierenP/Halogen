#pragma once

#include "datagen/encode.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string_view>

void viribin_adj(std::string_view input_path, std::string_view output_path)
{
    // The viribin is a binary file which contains N games. Each game starts with a 32 byte MarlinFormat header,
    // followed by M (move, eval) pairs. Each move is 2 bytes, and each eval is 2 bytes.
    // The file ends with a 4 byte terminator, which is all zeros.

    // We want to adjudicate each game
    constexpr static int win_adjudication_score = 750;
    constexpr static int win_adjudication_plys = 6;

    // Open the input file for reading
    std::ifstream input_file(std::string { input_path }, std::ios::binary);
    if (!input_file)
    {
        std::cerr << "Error: Could not open input file: " << input_path << std::endl;
        return;
    }

    // Open the output file for writing
    std::ofstream output_file(std::string { output_path }, std::ios::binary);
    if (!output_file)
    {
        std::cerr << "Error: Could not open output file: " << output_path << std::endl;
        return;
    }

    // Read the header
    MarlinFormat header;
    size_t games = 0;
    size_t moves = 0;

    size_t incorrect_game_results = 0;

    while (input_file.read(reinterpret_cast<char*>(&header), sizeof(header)))
    {
        // Write the header to the output file
        // output_file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        std::vector<std::pair<uint16_t, int16_t>> move_eval_pairs;
        int white_win_adj_count = 0;
        int black_win_adj_count = 0;
        bool game_adjudicated = false;

        // Read the moves and evals until we hit the terminator
        uint16_t move;
        int16_t eval;
        while (input_file.read(reinterpret_cast<char*>(&move), sizeof(move))
            && input_file.read(reinterpret_cast<char*>(&eval), sizeof(eval)))
        {
            if (move == 0 && eval == 0) // Check for the terminator
            {
                break; // Exit the loop if we hit the terminator
            }

            if (game_adjudicated)
            {
                continue;
            }

            move_eval_pairs.emplace_back(move, eval);
            moves++;

            // perform adjudication
            if (eval > win_adjudication_score)
            {
                white_win_adj_count++;
            }
            else
            {
                white_win_adj_count = 0;
            }

            if (eval < -win_adjudication_score)
            {
                black_win_adj_count++;
            }
            else
            {
                black_win_adj_count = 0;
            }

            if (white_win_adj_count >= win_adjudication_plys)
            {
                incorrect_game_results += (header.result != 2);
                header.result = 2;
                game_adjudicated = true;
            }
            else if (black_win_adj_count >= win_adjudication_plys)
            {
                incorrect_game_results += (header.result != 0);
                header.result = 0;
                game_adjudicated = true;
            }
        }

        // Write the header
        output_file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        // Write the moves
        for (const auto& [move_, eval_] : move_eval_pairs)
        {
            output_file.write(reinterpret_cast<const char*>(&move_), sizeof(move_));
            output_file.write(reinterpret_cast<const char*>(&eval_), sizeof(eval_));
        }

        // Write the terminator (4 bytes of zeros)
        uint8_t terminator[4] {};
        output_file.write(reinterpret_cast<const char*>(terminator), sizeof(terminator));

        games++;

        if (games % (1024 * 1024) == 0)
        {
            std::cout << "Processed " << games << " games with " << moves << " moves." << std::endl;
            std::cout << "Fraction of games adjudicated differently to original result: "
                      << float(incorrect_game_results) / float(games) * 100 << "%" << std::endl;
        }
    }

    std::cout << "Processed " << games << " games with " << moves << " moves." << std::endl;
    std::cout << "Fraction of games adjudicated differently to original result: "
              << float(incorrect_game_results) / float(games) * 100 << "%" << std::endl;

    // Close the files
    input_file.close();
    output_file.close();
}