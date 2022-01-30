#include "Zobrist.h"

#include <cstddef>
#include <random>

#include "Position.h"

const std::array<uint64_t, 781> Zobrist::ZobristTable = [] {
    std::array<uint64_t, 781> table;

    std::mt19937_64 gen(0);
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

    for (size_t i = 0; i < table.size(); i++)
    {
        table[i] = dist(gen);
    }

    return table;
}();

uint64_t Zobrist::Generate(const Position& position)
{
    uint64_t Key = EMPTY;

    for (int i = 0; i < N_PIECES; i++)
    {
        uint64_t bitboard = position.GetPieceBB(static_cast<Pieces>(i));
        while (bitboard != 0)
        {
            Key ^= ZobristTable.at(i * 64 + LSBpop(bitboard));
        }
    }

    if (position.GetTurn() == WHITE)
        Key ^= ZobristTable.at(12 * 64);

    if (position.GetCanCastleWhiteKingside())
        Key ^= ZobristTable.at(12 * 64 + 1);
    if (position.GetCanCastleWhiteQueenside())
        Key ^= ZobristTable.at(12 * 64 + 2);
    if (position.GetCanCastleBlackKingside())
        Key ^= ZobristTable.at(12 * 64 + 3);
    if (position.GetCanCastleBlackQueenside())
        Key ^= ZobristTable.at(12 * 64 + 4);

    if (position.GetEnPassant() <= SQ_H8)
    {
        Key ^= ZobristTable.at(12 * 64 + 5 + GetFile(position.GetEnPassant()));
    }

    return Key;
}