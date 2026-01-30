#include "search/zobrist.h"

#include "bitboard/define.h"
#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "utility/splitmix64.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace Zobrist
{

SplitMix64 rng(0);

const std::array<uint64_t, 12 * 64 + 1 + 16 + 8> Table = []
{
    std::array<uint64_t, 12 * 64 + 1 + 16 + 8> table;

    for (size_t i = 0; i < table.size(); i++)
    {
        table[i] = rng.next();
    }

    return table;
}();

uint64_t piece_square(Piece piece, Square square) noexcept
{
    return Table[piece * 64 + square];
}

uint64_t stm() noexcept
{
    return Table[12 * 64];
}

uint64_t castle(Square square) noexcept
{
    assert(enum_to<Rank>(square) == RANK_1 || enum_to<Rank>(square) == RANK_8);

    if (enum_to<Rank>(square) == RANK_1)
        return Table[12 * 64 + 1 + enum_to<File>(square)];
    else
        return Table[12 * 64 + 9 + enum_to<File>(square)];
}

uint64_t en_passant(File file) noexcept
{
    return Table[12 * 64 + 17 + file];
}

uint64_t key(const BoardState& board) noexcept
{
    uint64_t key = 0;

    for (int i = 0; i < N_PIECES; i++)
    {
        uint64_t bitboard = board.get_pieces_bb(static_cast<Piece>(i));
        while (bitboard != 0)
        {
            key ^= piece_square(static_cast<Piece>(i), lsbpop(bitboard));
        }
    }

    if (board.stm == WHITE)
    {
        key ^= stm();
    }

    auto castle_rights = board.castle_squares;
    while (castle_rights != 0)
    {
        key ^= castle(lsbpop(castle_rights));
    }

    if (board.en_passant <= SQ_H8)
    {
        key ^= en_passant(enum_to<File>(board.en_passant));
    }

    return key;
}

uint64_t pawn_key(const BoardState& board) noexcept
{
    uint64_t key = 0;

    uint64_t white = board.get_pieces_bb(WHITE_PAWN);
    while (white != 0)
    {
        key ^= piece_square(WHITE_PAWN, lsbpop(white));
    }

    uint64_t black = board.get_pieces_bb(BLACK_PAWN);
    while (black != 0)
    {
        key ^= piece_square(BLACK_PAWN, lsbpop(black));
    }

    return key;
}

uint64_t non_pawn_key(const BoardState& board, Side side) noexcept
{
    uint64_t key = 0;

    for (int i = KNIGHT; i <= KING; i++)
    {
        const auto piece = get_piece(static_cast<PieceType>(i), side);
        uint64_t bitboard = board.get_pieces_bb(piece);
        while (bitboard != 0)
        {
            key ^= piece_square(piece, lsbpop(bitboard));
        }
    }

    return key;
}

constexpr size_t fifty_move_buckets = 10;

const std::array<uint64_t, fifty_move_buckets> fifty_move_bucket_hash = []()
{
    std::array<uint64_t, fifty_move_buckets> table;
    for (size_t i = 0; i < fifty_move_buckets; i++)
    {
        table[i] = rng.next();
    }
    return table;
}();

size_t calculate_fifty_move_bucket(int fifty_move_count)
{
    return (fifty_move_count * 1024) / ((101 * 1024) / fifty_move_buckets);
}

uint64_t calculate_fifty_move_hash(int fifty_move_count)
{
    return fifty_move_bucket_hash[calculate_fifty_move_bucket(fifty_move_count)];
}

const auto fifty_move_hash = []()
{
    std::array<size_t, 101> buckets = {};
    for (int i = 0; i <= 100; i++)
    {
        buckets[i] = calculate_fifty_move_hash(i);
    }
    return buckets;
}();

uint64_t get_fifty_move_adj_key(const BoardState& board) noexcept
{
    assert(0 <= board.fifty_move_count && board.fifty_move_count <= 100);
    return board.key ^ fifty_move_hash[board.fifty_move_count];
}

} // namespace Zobrist
