#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <type_traits>

enum class SearchResultType : uint8_t
{
    EMPTY,
    EXACT,
    LOWER_BOUND,
    UPPER_BOUND,
};

enum Square : uint8_t
{
    // clang-format off
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,

    N_SQUARES,
    // clang-format on
};

constexpr Square& operator++(Square& sq)
{
    assert(sq < N_SQUARES);
    sq = static_cast<Square>(sq + 1);
    return sq;
}

enum Players : uint8_t
{
    BLACK,
    WHITE,

    N_PLAYERS
};

constexpr Players operator!(const Players& val)
{
    return val == WHITE ? BLACK : WHITE;
}

enum Pieces : uint8_t
{
    BLACK_PAWN,
    BLACK_KNIGHT,
    BLACK_BISHOP,
    BLACK_ROOK,
    BLACK_QUEEN,
    BLACK_KING,

    WHITE_PAWN,
    WHITE_KNIGHT,
    WHITE_BISHOP,
    WHITE_ROOK,
    WHITE_QUEEN,
    WHITE_KING,

    N_PIECES
};

enum PieceTypes : uint8_t
{
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,

    N_PIECE_TYPES
};

enum Rank : uint8_t
{
    RANK_1,
    RANK_2,
    RANK_3,
    RANK_4,
    RANK_5,
    RANK_6,
    RANK_7,
    RANK_8,

    N_RANKS
};

constexpr Rank Mirror(Rank rank)
{
    assert(rank < N_RANKS);
    return static_cast<Rank>(RANK_8 - rank);
}

constexpr Rank& operator++(Rank& rank)
{
    assert(rank < N_RANKS);
    rank = static_cast<Rank>(rank + 1);
    return rank;
}

enum File : uint8_t
{
    FILE_A,
    FILE_B,
    FILE_C,
    FILE_D,
    FILE_E,
    FILE_F,
    FILE_G,
    FILE_H,

    N_FILES
};

constexpr File& operator++(File& file)
{
    assert(file < N_FILES);
    file = static_cast<File>(file + 1);
    return file;
}

enum Diagonal : uint8_t
{
    DIAG_A8A8,
    DIAG_A7B8,
    DIAG_A6C8,
    DIAG_A5D8,
    DIAG_A4E8,
    DIAG_A3F8,
    DIAG_A2G8,
    DIAG_A1H8,
    DIAG_B1H7,
    DIAG_C1H6,
    DIAG_D1H5,
    DIAG_E1H4,
    DIAG_F1H3,
    DIAG_G1H2,
    DIAG_H1H1,

    N_DIAGONALS
};

constexpr Diagonal& operator++(Diagonal& diagonal)
{
    assert(diagonal < N_DIAGONALS);
    diagonal = static_cast<Diagonal>(diagonal + 1);
    return diagonal;
}

enum AntiDiagonal : uint8_t
{
    DIAG_H8H8,
    DIAG_G8H7,
    DIAG_F8H6,
    DIAG_E8H5,
    DIAG_D8H4,
    DIAG_C8H3,
    DIAG_B8H2,
    DIAG_A8H1,
    DIAG_A7G1,
    DIAG_A6F1,
    DIAG_A5E1,
    DIAG_A4D1,
    DIAG_A3C1,
    DIAG_A2B1,
    DIAG_A1A1,

    N_ANTI_DIAGONALS
};

constexpr AntiDiagonal& operator++(AntiDiagonal& antidiagonal)
{
    assert(antidiagonal < N_ANTI_DIAGONALS);
    antidiagonal = static_cast<AntiDiagonal>(antidiagonal + 1);
    return antidiagonal;
}

constexpr int GetBitCount(uint64_t bb)
{
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

constexpr Players ColourOfPiece(Pieces piece)
{
    assert(piece < N_PIECES);
    return static_cast<Players>((int)piece / (int)N_PIECE_TYPES);
}

constexpr PieceTypes GetPieceType(Pieces piece)
{
    return static_cast<PieceTypes>((int)piece % (int)N_PIECE_TYPES);
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
    return static_cast<AntiDiagonal>((int)RANK_8 + (int)FILE_H - (int)GetRank(square) - (int)GetFile(square));
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

    return static_cast<Pieces>((int)type + (int)N_PIECE_TYPES * (int)colour);
}

constexpr Square GetPosition(File file, Rank rank)
{
    assert(file < N_FILES);
    assert(rank < N_RANKS);

    return static_cast<Square>((int)rank * 8 + (int)file);
}

constexpr uint64_t EMPTY = 0;
constexpr uint64_t UNIVERSE = 0xffffffffffffffff;

namespace Detail // so these don't polute the global scope
{
// Not my code, slightly modified
constexpr uint64_t inBetween(unsigned int sq1, unsigned int sq2)
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

constexpr auto betweenArray = []()
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
            if (GetRank(i) == GetRank(j))
            {
                ret[i][j] = RankBB[GetRank(i)];
            }
            else if (GetFile(i) == GetFile(j))
            {
                ret[i][j] = FileBB[GetFile(i)];
            }
            else if (GetDiagonal(i) == GetDiagonal(j))
            {
                ret[i][j] = DiagonalBB[GetDiagonal(i)];
            }
            else if (GetAntiDiagonal(i) == GetAntiDiagonal(j))
            {
                ret[i][j] = AntiDiagonalBB[GetAntiDiagonal(i)];
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
            if ((AbsRankDiff(i, j) == 1 && AbsFileDiff(i, j) == 2)
                || (AbsRankDiff(i, j) == 2 && AbsFileDiff(i, j) == 1))
                ret[i] |= SquareBB[j];
    return ret;
}();

constexpr auto RookAttacks = []()
{
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = (RankBB[GetRank(i)] | FileBB[GetFile(i)]) ^ SquareBB[i];
    return ret;
}();

constexpr auto BishopAttacks = []()
{
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = (DiagonalBB[GetDiagonal(i)] | AntiDiagonalBB[GetAntiDiagonal(i)]) ^ SquareBB[i];
    return ret;
}();

constexpr auto KingAttacks = []()
{
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if (i != j && AbsFileDiff(i, j) <= 1 && AbsRankDiff(i, j) <= 1)
                ret[i] |= SquareBB[j];
    return ret;
}();

constexpr auto PawnAttacks = []()
{
    std::array<std::array<uint64_t, N_SQUARES>, N_PLAYERS> ret {};
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
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = RookAttacks[i] | BishopAttacks[i];
    return ret;
}();

constexpr Square LSB(uint64_t bb)
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

constexpr Square LSBpop(uint64_t& bb)
{
    assert(bb != 0);

    auto index = LSB(bb);
    bb &= bb - 1;

    return index;
}

constexpr Square MSB(uint64_t bb)
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
constexpr bool mayMove(Square from, Square to, uint64_t pieces)
{
    return (betweenArray[from][to] & pieces) == 0;
}

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