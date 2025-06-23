#pragma once

#include "bitboard/enum.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>

constexpr int popcount(uint64_t bb)
{
// TODO: use std::popcount?
#if defined(__GNUG__) && defined(USE_POPCNT)
    return __builtin_popcountll(bb);
#else
    // https://www.chessprogramming.org/Population_Count
    const uint64_t k1 = uint64_t(0x5555555555555555); /*  -1/3   */
    const uint64_t k2 = uint64_t(0x3333333333333333); /*  -1/5   */
    const uint64_t k4 = uint64_t(0x0f0f0f0f0f0f0f0f); /*  -1/17  */
    const uint64_t kf = uint64_t(0x0101010101010101); /*  -1/255 */
    bb = bb - ((bb >> 1) & k1); /* put count of each 2 bits into those 2 bits */
    bb = (bb & k2) + ((bb >> 2) & k2); /* put count of each 4 bits into those 4 bits */
    bb = (bb + (bb >> 4)) & k4; /* put count of each 8 bits into those 8 bits */
    bb = (bb * kf) >> 56; /* returns 8 most significant bits of x + (x<<8) + (x<<16) + (x<<24) + ...  */
    return (int)bb;
#endif
}

constexpr uint64_t EMPTY = 0;
constexpr uint64_t UNIVERSE = 0xffffffffffffffff;

namespace Detail // so these don't polute the global scope
{
// Not my code, slightly modified
constexpr uint64_t inBetween(int sq1, int sq2)
{
    const uint64_t a2a7 = uint64_t(0x0001010101010100);
    const uint64_t b2g7 = uint64_t(0x0040201008040200);
    const uint64_t h1b7 = uint64_t(0x0002040810204080); /* Thanks Dustin, g2b7 did not work for c1-a3 */
    uint64_t btwn = 0, line = 0, rank = 0, file = 0;

    btwn = (UNIVERSE << sq1) ^ (UNIVERSE << sq2);
    file = (sq2 & 7) - (sq1 & 7);
    rank = ((sq2 | 7) - sq1) >> 3;
    line = ((file & 7) - 1) & a2a7; /* a2a7 if same file */
    line += 2 * (((rank & 7) - 1) >> 58); /* b1g1 if same rank */
    line += (((rank - file) & 15) - 1) & b2g7; /* b2g7 if same diagonal */
    line += (((rank + file) & 15) - 1) & h1b7; /* h1b7 if same antidiag */
    line = int64_t(uint64_t(line) << (std::min)(sq1, sq2)); /* shift by smaller square */
    return line & btwn; /* return the bits on that line in-between */
}
}

constexpr auto RankBB = []()
{
    std::array<uint64_t, N_RANKS> ret {};
    for (Rank i = RANK_1; i <= RANK_8; ++i)
        ret[i] = 0xffULL << (8 * i);
    return ret;
}();

constexpr auto FileBB = []()
{
    std::array<uint64_t, N_FILES> ret {};
    for (File i = FILE_A; i <= FILE_H; ++i)
        ret[i] = 0x101010101010101 << i;
    return ret;
}();

constexpr auto SquareBB = []()
{
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = 1ULL << i;
    return ret;
}();

constexpr auto DiagonalBB = []()
{
    std::array<uint64_t, N_DIAGONALS> ret { 0x100000000000000 };
    for (Diagonal i = DIAG_A7B8; i <= DIAG_H1H1; ++i)
        if (i > N_DIAGONALS / 2)
            ret[i] = (ret[i - 1] >> 8);
        else
            ret[i] = (ret[i - 1] << 1) | (ret[i - 1] >> 8);
    return ret;
}();

constexpr auto AntiDiagonalBB = []()
{
    std::array<uint64_t, N_ANTI_DIAGONALS> ret { 0x8000000000000000 };
    for (AntiDiagonal i = DIAG_G8H7; i <= DIAG_A1A1; ++i)
        if (i > N_ANTI_DIAGONALS / 2)
            ret[i] = (ret[i - 1] >> 8);
        else
            ret[i] = (ret[i - 1] >> 1) | (ret[i - 1] >> 8);
    return ret;
}();

constexpr auto BetweenBB = []()
{
    std::array<std::array<uint64_t, N_SQUARES>, N_SQUARES> ret {};
    for (int i = 0; i < 64; i++)
    {
        for (int j = 0; j < 64; j++)
        {
            ret[i][j] = Detail::inBetween(i, j);
        }
    }
    return ret;
}();

constexpr auto RayBB = []()
{
    std::array<std::array<uint64_t, N_SQUARES>, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
    {
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
        {
            if (enum_to<Rank>(i) == enum_to<Rank>(j))
            {
                ret[i][j] = RankBB[enum_to<Rank>(i)];
            }
            else if (enum_to<File>(i) == enum_to<File>(j))
            {
                ret[i][j] = FileBB[enum_to<File>(i)];
            }
            else if (enum_to<Diagonal>(i) == enum_to<Diagonal>(j))
            {
                ret[i][j] = DiagonalBB[enum_to<Diagonal>(i)];
            }
            else if (enum_to<AntiDiagonal>(i) == enum_to<AntiDiagonal>(j))
            {
                ret[i][j] = AntiDiagonalBB[enum_to<AntiDiagonal>(i)];
            }
        }
    }
    return ret;
}();

constexpr auto KnightAttacks = []()
{
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if ((abs_rank_diff(i, j) == 1 && abs_file_diff(i, j) == 2)
                || (abs_rank_diff(i, j) == 2 && abs_file_diff(i, j) == 1))
                ret[i] |= SquareBB[j];
    return ret;
}();

constexpr auto RookAttacks = []()
{
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = (RankBB[enum_to<Rank>(i)] | FileBB[enum_to<File>(i)]) ^ SquareBB[i];
    return ret;
}();

constexpr auto BishopAttacks = []()
{
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = (DiagonalBB[enum_to<Diagonal>(i)] | AntiDiagonalBB[enum_to<AntiDiagonal>(i)]) ^ SquareBB[i];
    return ret;
}();

constexpr auto KingAttacks = []()
{
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if (i != j && abs_file_diff(i, j) <= 1 && abs_rank_diff(i, j) <= 1)
                ret[i] |= SquareBB[j];
    return ret;
}();

constexpr auto PawnAttacks = []()
{
    std::array<std::array<uint64_t, N_SQUARES>, N_SIDES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if ((abs_file_diff(i, j) == 1) && rank_diff(j, i) == 1) // either side one ahead
                ret[WHITE][i] |= SquareBB[j];
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if ((abs_file_diff(i, j) == 1) && rank_diff(j, i) == -1) // either side one behind
                ret[BLACK][i] |= SquareBB[j];
    return ret;
}();

constexpr auto QueenAttacks = []()
{
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = RookAttacks[i] | BishopAttacks[i];
    return ret;
}();

constexpr Square lsb(uint64_t bb)
{
    assert(bb != 0);

#if defined(__GNUG__) && defined(USE_POPCNT)
    return static_cast<Square>(__builtin_ctzll(bb));
#else
    /**
     * bitScanForward
     * @author Kim Walisch (2012)
     * @param bb bitboard to scan
     * @precondition bb != 0
     * @return index (0..63) of least significant one bit
     */
    // clang-format off
    constexpr std::array<int, N_SQUARES> index64 = {
        0, 47, 1, 56, 48, 27, 2, 60,
        57, 49, 41, 37, 28, 16, 3, 61,
        54, 58, 35, 52, 50, 42, 21, 44,
        38, 32, 29, 23, 17, 11, 4, 62,
        46, 55, 26, 59, 40, 36, 15, 53,
        34, 51, 20, 43, 31, 22, 10, 45,
        25, 39, 14, 33, 19, 30, 9, 24,
        13, 18, 8, 12, 7, 6, 5, 63
    };
    // clang-format on
    constexpr uint64_t debruijn64 = uint64_t(0x03f79d71b4cb0a89);
    return static_cast<Square>(index64[((bb ^ (bb - 1)) * debruijn64) >> 58]);
#endif
}

constexpr Square lsbpop(uint64_t& bb)
{
    assert(bb != 0);

    auto index = lsb(bb);
    bb &= bb - 1;

    return index;
}

constexpr Square msb(uint64_t bb)
{
    assert(bb != 0);

#if defined(__GNUG__) && defined(USE_POPCNT)
    return static_cast<Square>(SQ_H8 - __builtin_clzll(bb));
#else
    /**
     * bitScanReverse
     * @authors Kim Walisch, Mark Dickinson
     * @param bb bitboard to scan
     * @precondition bb != 0
     * @return index (0..63) of most significant one bit
     */
    // clang-format off
    constexpr std::array<int, N_SQUARES> index64 = {
        0, 47, 1, 56, 48, 27, 2, 60,
        57, 49, 41, 37, 28, 16, 3, 61,
        54, 58, 35, 52, 50, 42, 21, 44,
        38, 32, 29, 23, 17, 11, 4, 62,
        46, 55, 26, 59, 40, 36, 15, 53,
        34, 51, 20, 43, 31, 22, 10, 45,
        25, 39, 14, 33, 19, 30, 9, 24,
        13, 18, 8, 12, 7, 6, 5, 63
    };
    // clang-format on
    constexpr uint64_t debruijn64 = uint64_t(0x03f79d71b4cb0a89);
    bb |= bb >> 1;
    bb |= bb >> 2;
    bb |= bb >> 4;
    bb |= bb >> 8;
    bb |= bb >> 16;
    bb |= bb >> 32;
    return static_cast<Square>(index64[(bb * debruijn64) >> 58]);
#endif
}

constexpr bool path_clear(Square from, Square to, uint64_t pieces)
{
    return (BetweenBB[from][to] & pieces) == 0;
}

// TODO: MAX_DEPTH and MAX_RECURSION should be defined separately
const int MAX_DEPTH = 100;

enum class Shift : int8_t
{
    // clang-format off
            NN = 16,
    NW =  7, N =  8, NE =  9,
     W = -1,          E =  1,
    SW = -9, S = -8, SE = -7,
            SS = -16
    // clang-format on
};

constexpr Square operator+(Square sq, Shift s)
{
    return static_cast<Square>((int)sq + (int)s);
};

constexpr Square operator-(Square sq, Shift s)
{
    return static_cast<Square>((int)sq - (int)s);
};

template <Shift direction>
constexpr auto shift_bb(uint64_t bb);

template <>
constexpr auto shift_bb<Shift::N>(uint64_t bb)
{
    return bb << 8;
}

template <>
constexpr auto shift_bb<Shift::S>(uint64_t bb)
{
    return bb >> 8;
}

template <>
constexpr auto shift_bb<Shift::W>(uint64_t bb)
{
    return (bb & ~FileBB[FILE_A]) >> 1;
}

template <>
constexpr auto shift_bb<Shift::E>(uint64_t bb)
{
    return (bb & ~FileBB[FILE_H]) << 1;
}

template <>
constexpr auto shift_bb<Shift::NW>(uint64_t bb)
{
    return (bb & ~FileBB[FILE_A]) << 7;
}

template <>
constexpr auto shift_bb<Shift::NE>(uint64_t bb)
{
    return (bb & ~FileBB[FILE_H]) << 9;
}

template <>
constexpr auto shift_bb<Shift::SW>(uint64_t bb)
{
    return (bb & ~FileBB[FILE_A]) >> 9;
}

template <>
constexpr auto shift_bb<Shift::SE>(uint64_t bb)
{
    return (bb & ~FileBB[FILE_H]) >> 7;
}

static_assert(shift_bb<Shift::N>(SquareBB[SQ_E4]) == SquareBB[SQ_E5]);
static_assert(shift_bb<Shift::S>(SquareBB[SQ_E4]) == SquareBB[SQ_E3]);
static_assert(shift_bb<Shift::W>(SquareBB[SQ_E4]) == SquareBB[SQ_D4]);
static_assert(shift_bb<Shift::E>(SquareBB[SQ_E4]) == SquareBB[SQ_F4]);

static_assert(shift_bb<Shift::NW>(SquareBB[SQ_E4]) == SquareBB[SQ_D5]);
static_assert(shift_bb<Shift::NE>(SquareBB[SQ_E4]) == SquareBB[SQ_F5]);
static_assert(shift_bb<Shift::SW>(SquareBB[SQ_E4]) == SquareBB[SQ_D3]);
static_assert(shift_bb<Shift::SE>(SquareBB[SQ_E4]) == SquareBB[SQ_F3]);