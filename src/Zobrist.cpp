#include "Zobrist.h"

#include <cstddef>
#include <random>

#include "BitBoardDefine.h"
#include "BoardState.h"

const std::array<uint64_t, 12 * 64 + 1 + 16 + 8> Zobrist::ZobristTable = []
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

const std::array<uint64_t, 2 * 64> PawnKey::table = []
{
    std::array<uint64_t, 2 * 64> table;

    std::mt19937_64 gen(0);
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

    for (size_t i = 0; i < table.size(); i++)
    {
        table[i] = dist(gen);
    }

    return table;
}();

void Zobrist::ToggleCastle(Square sq)
{
    assert(GetRank(sq) == RANK_1 || GetRank(sq) == RANK_8);

    if (GetRank(sq) == RANK_1)
        key ^= ZobristTable[12 * 64 + 1 + GetFile(sq)];
    else
        key ^= ZobristTable[12 * 64 + 9 + GetFile(sq)];
}

uint64_t Zobrist::Generate(const BoardState& board)
{
    Zobrist Key;

    for (int i = 0; i < N_PIECES; i++)
    {
        uint64_t bitboard = board.GetPieceBB(static_cast<Pieces>(i));
        while (bitboard != 0)
        {
            Key.TogglePieceSquare(static_cast<Pieces>(i), LSBpop(bitboard));
        }
    }

    if (board.stm == WHITE)
        Key.ToggleSTM();

    auto castle_rights = board.castle_squares;
    while (castle_rights != 0)
    {
        Key.ToggleCastle(LSBpop(castle_rights));
    }

    if (board.en_passant <= SQ_H8)
    {
        Key.ToggleEnpassant(GetFile(board.en_passant));
    }

    return Key.Key();
}

uint64_t PawnKey::generate(const BoardState& board)
{
    PawnKey Key;

    uint64_t white = board.GetPieceBB<WHITE_PAWN>();
    while (white != 0)
    {
        Key.toggle_piece_square(WHITE_PAWN, LSBpop(white));
    }

    uint64_t black = board.GetPieceBB<BLACK_PAWN>();
    while (black != 0)
    {
        Key.toggle_piece_square(BLACK_PAWN, LSBpop(black));
    }

    return Key;
}