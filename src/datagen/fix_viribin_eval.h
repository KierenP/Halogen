#pragma once

#include "datagen/encode.h"

#include <fstream>
#include <iostream>
#include <string_view>

void fix_viribin_eval(std::string_view input_path, std::string_view output_path)
{
    // The viribin is a binary file which contains N games. Each game starts with a 32 byte MarlinFormat header,
    // followed by M (move, eval) pairs. Each move is 2 bytes, and each eval is 2 bytes.
    // The file ends with a 4 byte terminator, which is all zeros.

    // Due to a bug in datagen14/15, the evals for the black side were incorrectly stored as white side evals.
    // This function reads the input file, fixes the evals, and writes the output file

    // Open the input file for reading
    std::ifstream input_file(input_path.data(), std::ios::binary);
    if (!input_file)
    {
        std::cerr << "Error: Could not open input file: " << input_path << std::endl;
        return;
    }

    // Open the output file for writing
    std::ofstream output_file(output_path.data(), std::ios::binary);
    if (!output_file)
    {
        std::cerr << "Error: Could not open output file: " << output_path << std::endl;
        return;
    }

    // Read the header
    MarlinFormat header;

    while (input_file.read(reinterpret_cast<char*>(&header), sizeof(header)))
    {
        // Write the header to the output file
        output_file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        Side stm = (header.stm_ep & 0x80) ? BLACK : WHITE;

        // Read the moves and evals until we hit the terminator
        uint16_t move;
        int16_t eval;
        while (input_file.read(reinterpret_cast<char*>(&move), sizeof(move))
            && input_file.read(reinterpret_cast<char*>(&eval), sizeof(eval)))
        {
            if (stm == BLACK) // Check if the move is for black
            {
                eval = -eval;
            }

            // Write the move and eval to the output file
            output_file.write(reinterpret_cast<const char*>(&move), sizeof(move));
            output_file.write(reinterpret_cast<const char*>(&eval), sizeof(eval));
            stm = !stm; // Switch sides for the next move
        }

        // Write the terminator (4 bytes of zeros)
        uint8_t terminator[4] = { 0, 0, 0, 0 };
        output_file.write(reinterpret_cast<const char*>(terminator), sizeof(terminator));
    }

    // Close the files
    input_file.close();
    output_file.close();
}