#include "Zobrist.h"

#include <cstddef>
#include <random>

#include "BoardState.h"

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

uint64_t Zobrist::Generate(const BoardState& board)
{
    uint64_t Key = EMPTY;

    for (int i = 0; i < N_PIECES; i++)
    {
        uint64_t bitboard = board.GetPieceBB(static_cast<Pieces>(i));
        while (bitboard != 0)
        {
            Key ^= ZobristTable.at(i * 64 + LSBpop(bitboard));
        }
    }

    if (board.stm == WHITE)
        Key ^= ZobristTable.at(12 * 64);

    if (board.white_king_castle)
        Key ^= ZobristTable.at(12 * 64 + 1);
    if (board.white_queen_castle)
        Key ^= ZobristTable.at(12 * 64 + 2);
    if (board.black_king_castle)
        Key ^= ZobristTable.at(12 * 64 + 3);
    if (board.black_queen_castle)
        Key ^= ZobristTable.at(12 * 64 + 4);

    if (board.en_passant <= SQ_H8)
    {
        Key ^= ZobristTable.at(12 * 64 + 5 + GetFile(board.en_passant));
    }

    return Key;
}