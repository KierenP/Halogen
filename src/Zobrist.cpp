#include "Zobrist.h"

#include <cassert>
#include <cstddef>
#include <random>

#include "BitBoardDefine.h"
#include "BoardState.h"

namespace Zobrist
{

const std::array<uint64_t, 12 * 64 + 1 + 16 + 8> Table = []
{
    std::array<uint64_t, 12 * 64 + 1 + 16 + 8> table;

    std::mt19937_64 gen(0);
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

    for (size_t i = 0; i < table.size(); i++)
    {
        table[i] = dist(gen);
    }

    return table;
}();

uint64_t piece_square(Pieces piece, Square square)
{
    return Table[piece * 64 + square];
}

uint64_t stm()
{
    return Table[12 * 64];
}

uint64_t castle(Square square)
{
    assert(GetRank(square) == RANK_1 || GetRank(square) == RANK_8);

    if (GetRank(square) == RANK_1)
        return Table[12 * 64 + 1 + GetFile(square)];
    else
        return Table[12 * 64 + 9 + GetFile(square)];
}

uint64_t en_passant(File file)
{
    return Table[12 * 64 + 17 + file];
}

uint64_t key(const BoardState& board)
{
    uint64_t key = 0;

    for (int i = 0; i < N_PIECES; i++)
    {
        uint64_t bitboard = board.GetPieceBB(static_cast<Pieces>(i));
        while (bitboard != 0)
        {
            key ^= piece_square(static_cast<Pieces>(i), LSBpop(bitboard));
        }
    }

    if (board.stm == WHITE)
    {
        key ^= stm();
    }

    auto castle_rights = board.castle_squares;
    while (castle_rights != 0)
    {
        key ^= castle(LSBpop(castle_rights));
    }

    if (board.en_passant <= SQ_H8)
    {
        key ^= en_passant(GetFile(board.en_passant));
    }

    return key;
}

uint64_t pawn_key(const BoardState& board)
{
    uint64_t key = 0;

    uint64_t white = board.GetPieceBB<WHITE_PAWN>();
    while (white != 0)
    {
        key ^= piece_square(WHITE_PAWN, LSBpop(white));
    }

    uint64_t black = board.GetPieceBB<BLACK_PAWN>();
    while (black != 0)
    {
        key ^= piece_square(BLACK_PAWN, LSBpop(black));
    }

    return key;
}

} // namespace Zobrist
