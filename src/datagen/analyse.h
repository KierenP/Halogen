#pragma once

#include "datagen/encode.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string_view>

inline void analyse_viribin(std::string_view input_path)
{
    // The viribin is a binary file which contains N games. Each game starts with a 32 byte MarlinFormat header,
    // followed by M (move, eval) pairs. Each move is 2 bytes, and each eval is 2 bytes. Each game ends with a 4 byte
    // terminator, which is all zeros.

    // Open the input file for reading
    std::ifstream input_file(std::string { input_path }, std::ios::binary);
    if (!input_file)
    {
        std::cerr << "Error: Could not open input file: " << input_path << std::endl;
        return;
    }

    // Read the header
    MarlinFormat header;
    size_t games = 0;
    size_t moves = 0;

    while (input_file.read(reinterpret_cast<char*>(&header), sizeof(header)))
    {
        Side stm = (header.stm_ep & 0x80) ? BLACK : WHITE;

        // Read the moves and evals until we hit the terminator
        uint16_t move;
        int16_t eval;
        while (input_file.read(reinterpret_cast<char*>(&move), sizeof(move))
            && input_file.read(reinterpret_cast<char*>(&eval), sizeof(eval)))
        {
            if (move == 0 && eval == 0) // Check for the terminator (4 bytes of zeros)
            {
                break; // Exit the loop if we hit the terminator
            }

            if (stm == BLACK) // Check if the move is for black
            {
                eval = -eval;
            }

            stm = !stm; // Switch sides for the next move
            moves++;
        }

        games++;
    }

    std::cout << "Processed " << games << " games with " << moves << " moves." << std::endl;

    // Close the files
    input_file.close();
}