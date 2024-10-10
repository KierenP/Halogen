#include "Zobrist.h"

#include <cstddef>
#include <random>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Utility.h"

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
        auto piece = static_cast<Pieces>(i);
        extract_bits(board.GetPieceBB(piece), [&](auto sq) { Key.TogglePieceSquare(piece, sq); });
    }

    if (board.stm == WHITE)
        Key.ToggleSTM();

    extract_bits(board.castle_squares, [&](auto sq) { Key.ToggleCastle(sq); });

    if (board.en_passant <= SQ_H8)
    {
        Key.ToggleEnpassant(GetFile(board.en_passant));
    }

    return Key.Key();
}

uint64_t PawnKey::generate(const BoardState& board)
{
    PawnKey Key;

    extract_bits(board.GetPieceBB<WHITE_PAWN>(), [&](auto sq) { Key.toggle_piece_square(WHITE_PAWN, sq); });
    extract_bits(board.GetPieceBB<BLACK_PAWN>(), [&](auto sq) { Key.toggle_piece_square(BLACK_PAWN, sq); });

    return Key;
}