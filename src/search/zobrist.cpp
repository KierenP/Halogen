#include "search/zobrist.h"

#include "bitboard/define.h"
#include "bitboard/enum.h"
#include "chessboard/board_state.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <random>

namespace Zobrist
{
uint32_t random_state = 180428938;

// generate 32-bit pseudo legal numbers
uint32_t get_random_U32_number(void)
{
    // get current state
    uint32_t number = random_state;

    // XOR shift algorithm
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    // update random number state
    random_state = number;

    // return random number
    return number;
}

// generate 64-bit pseudo legal numbers
uint64_t get_random_uint64_number(void)
{
    // define 4 random numbers
    uint64_t n1, n2, n3, n4;

    // init random numbers slicing 16 bits from MS1B side
    n1 = (uint64_t)(get_random_U32_number()) & 0xFFFF;
    n2 = (uint64_t)(get_random_U32_number()) & 0xFFFF;
    n3 = (uint64_t)(get_random_U32_number()) & 0xFFFF;
    n4 = (uint64_t)(get_random_U32_number()) & 0xFFFF;

    // return random number
    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

const std::array<uint64_t, 12 * 64 + 1 + 16 + 8> Table = []
{
    std::array<uint64_t, 12 * 64 + 1 + 16 + 8> table;

    for (size_t i = 0; i < table.size(); i++)
    {
        table[i] = get_random_uint64_number();
    }

    return table;
}();

uint64_t piece_square(Piece piece, Square square)
{
    return Table[piece * 64 + square];
}

uint64_t stm()
{
    return Table[12 * 64];
}

uint64_t castle(Square square)
{
    assert(enum_to<Rank>(square) == RANK_1 || enum_to<Rank>(square) == RANK_8);

    if (enum_to<Rank>(square) == RANK_1)
        return Table[12 * 64 + 1 + enum_to<File>(square)];
    else
        return Table[12 * 64 + 9 + enum_to<File>(square)];
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

uint64_t pawn_key(const BoardState& board)
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

uint64_t non_pawn_key(const BoardState& board, Side side)
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

uint64_t get_fifty_move_adj_key(const BoardState& board)
{
    return board.key;
}

} // namespace Zobrist
