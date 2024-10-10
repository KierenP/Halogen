#pragma once

#include "Bitboard.h"
#include <algorithm>
#include <array>
#include <assert.h>
#include <cstdint>
#include <type_traits>

constexpr int GetBitCount(BB bb)
{
#if defined(__GNUG__) && defined(USE_POPCNT)
    return __builtin_popcountll(uint64_t(bb));
#else
    // https://www.chessprogramming.org/Population_Count
    uint64_t bbull = (uint64_t)bb;
    constexpr uint64_t k1 { 0x5555555555555555 }; /*  -1/3   */
    constexpr uint64_t k2 { 0x3333333333333333 }; /*  -1/5   */
    constexpr uint64_t k4 { 0x0f0f0f0f0f0f0f0f }; /*  -1/17  */
    constexpr uint64_t kf { 0x0101010101010101 }; /*  -1/255 */
    bbull = bbull - ((bbull >> 1) & k1); /* put count of each 2 bits into those 2 bits */
    bbull = (bbull & k2) + ((bbull >> 2) & k2); /* put count of each 4 bits into those 4 bits */
    bbull = (bbull + (bbull >> 4)) & k4; /* put count of each 8 bits into those 8 bits */
    bbull = (bbull * kf) >> 56; /* returns 8 most significant bits of x + (x<<8) + (x<<16) + (x<<24) + ...  */
    return (uint64_t)bbull;
#endif
}

constexpr Players ColourOfPiece(Pieces piece)
{
    assert(piece < N_PIECES);
    return static_cast<Players>(piece / N_PIECE_TYPES);
}

constexpr PieceTypes GetPieceType(Pieces piece)
{
    return static_cast<PieceTypes>(piece % N_PIECE_TYPES);
}

constexpr File GetFile(Square square)
{
    assert(square < N_SQUARES);
    return static_cast<File>(square % 8);
}

constexpr Rank GetRank(Square square)
{
    assert(square < N_SQUARES);
    return static_cast<Rank>(square / 8);
}

constexpr Diagonal GetDiagonal(Square square)
{
    assert(square < N_SQUARES);
    return static_cast<Diagonal>((RANK_8 - GetRank(square)) + GetFile(square));
}

constexpr AntiDiagonal GetAntiDiagonal(Square square)
{
    assert(square < N_SQUARES);
    return static_cast<AntiDiagonal>(RANK_8 + FILE_H - GetRank(square) - GetFile(square));
}

constexpr int RankDiff(Square sq1, Square sq2)
{
    assert(sq1 < N_SQUARES && sq2 < N_SQUARES);
    return static_cast<int>(GetRank(sq1)) - static_cast<int>(GetRank(sq2));
}

constexpr int FileDiff(Square sq1, Square sq2)
{
    assert(sq1 < N_SQUARES && sq2 < N_SQUARES);
    return static_cast<int>(GetFile(sq1)) - static_cast<int>(GetFile(sq2));
}

// std::abs is not constexpr (this is being added to the standard soon)
// for now, define my own
template <class T, std::enable_if_t<std::is_arithmetic_v<T>>...>
constexpr auto abs_constexpr(T const& x) noexcept
{
    return x < 0 ? -x : x;
}

constexpr unsigned int AbsRankDiff(Square sq1, Square sq2)
{
    assert(sq1 < N_SQUARES);
    assert(sq2 < N_SQUARES);

    return abs_constexpr(static_cast<int>(GetRank(sq1)) - static_cast<int>(GetRank(sq2)));
}

constexpr unsigned int AbsFileDiff(Square sq1, Square sq2)
{
    assert(sq1 < N_SQUARES);
    assert(sq2 < N_SQUARES);

    return abs_constexpr(static_cast<int>(GetFile(sq1)) - static_cast<int>(GetFile(sq2)));
}

constexpr Pieces Piece(PieceTypes type, Players colour)
{
    assert(type < N_PIECE_TYPES);
    assert(colour < N_PLAYERS);

    return static_cast<Pieces>(type + N_PIECE_TYPES * colour);
}

constexpr Square GetPosition(File file, Rank rank)
{
    assert(file < N_FILES);
    assert(rank < N_RANKS);

    return static_cast<Square>(rank * 8 + file);
}

namespace Detail // so these don't polute the global scope
{
// Not my code, slightly modified
constexpr BB inBetween(unsigned int sq1, unsigned int sq2)
{
    const uint64_t a2a7 = uint64_t(0x0001010101010100);
    const uint64_t b2g7 = uint64_t(0x0040201008040200);
    const uint64_t h1b7 = uint64_t(0x0002040810204080); /* Thanks Dustin, g2b7 did not work for c1-a3 */
    uint64_t btwn = 0, line = 0, rank = 0, file = 0;

    btwn = uint64_t((BB::all << sq1) ^ (BB::all << sq2));
    file = (sq2 & 7) - (sq1 & 7);
    rank = ((sq2 | 7) - sq1) >> 3;
    line = ((file & 7) - 1) & a2a7; /* a2a7 if same file */
    line += 2 * (((rank & 7) - 1) >> 58); /* b1g1 if same rank */
    line += (((rank - file) & 15) - 1) & b2g7; /* b2g7 if same diagonal */
    line += (((rank + file) & 15) - 1) & h1b7; /* h1b7 if same antidiag */
    line = int64_t(uint64_t(line) << (std::min)(sq1, sq2)); /* shift by smaller square */
    return BB { line & btwn }; /* return the bits on that line in-between */
}
}

constexpr auto RankBB = []()
{
    std::array<BB, N_RANKS> ret {};
    for (Rank i = RANK_1; i <= RANK_8; ++i)
        ret[i] = BB { 0xffULL } << (8 * i);
    return ret;
}();

constexpr auto FileBB = []()
{
    std::array<BB, N_FILES> ret {};
    for (File i = FILE_A; i <= FILE_H; ++i)
        ret[i] = BB { 0x101010101010101 } << i;
    return ret;
}();

constexpr auto SquareBB = []()
{
    std::array<BB, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = BB { 1ULL } << i;
    return ret;
}();

constexpr auto DiagonalBB = []()
{
    std::array<BB, N_DIAGONALS> ret { BB { 0x100000000000000 } };
    for (Diagonal i = DIAG_A7B8; i <= DIAG_H1H1; ++i)
        if (i > N_DIAGONALS / 2)
            ret[i] = (ret[i - 1] >> 8);
        else
            ret[i] = (ret[i - 1] << 1) | (ret[i - 1] >> 8);
    return ret;
}();

constexpr auto AntiDiagonalBB = []()
{
    std::array<BB, N_ANTI_DIAGONALS> ret { BB { 0x8000000000000000 } };
    for (AntiDiagonal i = DIAG_G8H7; i <= DIAG_A1A1; ++i)
        if (i > N_ANTI_DIAGONALS / 2)
            ret[i] = (ret[i - 1] >> 8);
        else
            ret[i] = (ret[i - 1] >> 1) | (ret[i - 1] >> 8);
    return ret;
}();

constexpr auto betweenArray = []()
{
    std::array<std::array<BB, N_SQUARES>, N_SQUARES> ret {};
    for (int i = 0; i < 64; i++)
    {
        for (int j = 0; j < 64; j++)
        {
            ret[i][j] = Detail::inBetween(i, j);
        }
    }
    return ret;
}();

constexpr auto KnightAttacks = []()
{
    std::array<BB, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if ((AbsRankDiff(i, j) == 1 && AbsFileDiff(i, j) == 2)
                || (AbsRankDiff(i, j) == 2 && AbsFileDiff(i, j) == 1))
                ret[i] |= SquareBB[j];
    return ret;
}();

constexpr auto RookAttacks = []()
{
    std::array<BB, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = (RankBB[GetRank(i)] | FileBB[GetFile(i)]) ^ SquareBB[i];
    return ret;
}();

constexpr auto BishopAttacks = []()
{
    std::array<BB, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = (DiagonalBB[GetDiagonal(i)] | AntiDiagonalBB[GetAntiDiagonal(i)]) ^ SquareBB[i];
    return ret;
}();

constexpr auto KingAttacks = []()
{
    std::array<BB, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if (i != j && AbsFileDiff(i, j) <= 1 && AbsRankDiff(i, j) <= 1)
                ret[i] |= SquareBB[j];
    return ret;
}();

constexpr auto PawnAttacks = []()
{
    std::array<std::array<BB, N_SQUARES>, N_PLAYERS> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if ((AbsFileDiff(i, j) == 1) && RankDiff(j, i) == 1) // either side one ahead
                ret[WHITE][i] |= SquareBB[j];
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if ((AbsFileDiff(i, j) == 1) && RankDiff(j, i) == -1) // either side one behind
                ret[BLACK][i] |= SquareBB[j];
    return ret;
}();

constexpr auto QueenAttacks = []()
{
    std::array<BB, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = RookAttacks[i] | BishopAttacks[i];
    return ret;
}();

constexpr Square LSB(BB bb)
{
    assert(bb != BB::none);
    uint64_t bbull = uint64_t(bb);

#if defined(__GNUG__) && defined(USE_POPCNT)
    return static_cast<Square>(__builtin_ctzll(bbull));
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
    return static_cast<Square>(index64[((bbull ^ (bbull - 1)) * debruijn64) >> 58]);
#endif
}

constexpr Square LSBpop(BB& bb)
{
    assert(bb != BB::none);

    auto index = LSB(bb);
    bb &= BB { (uint64_t)bb - 1 };

    return index;
}

constexpr Square MSB(BB bb)
{
    assert(bb != BB::none);
    uint64_t bbull = uint64_t(bb);

#if defined(__GNUG__) && defined(USE_POPCNT)
    return static_cast<Square>(SQ_H8 - __builtin_clzll(bbull));
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
    bbull |= bbull >> 1;
    bbull |= bbull >> 2;
    bbull |= bbull >> 4;
    bbull |= bbull >> 8;
    bbull |= bbull >> 16;
    bbull |= bbull >> 32;
    return static_cast<Square>(index64[(bbull * debruijn64) >> 58]);
#endif
}
constexpr bool mayMove(Square from, Square to, BB pieces)
{
    return (betweenArray[from][to] & pieces) == BB::none;
}

const int MAX_DEPTH = 100;

enum class Shift
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
constexpr auto shift_bb(BB bb);

template <>
constexpr auto shift_bb<Shift::N>(BB bb)
{
    return bb << 8;
}

template <>
constexpr auto shift_bb<Shift::S>(BB bb)
{
    return bb >> 8;
}

template <>
constexpr auto shift_bb<Shift::W>(BB bb)
{
    return (bb & ~FileBB[FILE_A]) >> 1;
}

template <>
constexpr auto shift_bb<Shift::E>(BB bb)
{
    return (bb & ~FileBB[FILE_H]) << 1;
}

template <>
constexpr auto shift_bb<Shift::NW>(BB bb)
{
    return (bb & ~FileBB[FILE_A]) << 7;
}

template <>
constexpr auto shift_bb<Shift::NE>(BB bb)
{
    return (bb & ~FileBB[FILE_H]) << 9;
}

template <>
constexpr auto shift_bb<Shift::SW>(BB bb)
{
    return (bb & ~FileBB[FILE_A]) >> 9;
}

template <>
constexpr auto shift_bb<Shift::SE>(BB bb)
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