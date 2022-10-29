#pragma once

#include <algorithm>
#include <array>
#include <assert.h>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

enum Square
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

enum Players
{
    BLACK,
    WHITE,

    N_PLAYERS
};

constexpr Players operator!(const Players& val)
{
    return val == WHITE ? BLACK : WHITE;
}

enum Pieces
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

enum PieceTypes
{
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,

    N_PIECE_TYPES
};

enum Rank
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

enum File
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

enum Diagonal
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

enum AntiDiagonal
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

enum Score
{
    HighINF = 30000,
    LowINF = -30000,

    MATED = -10000,
    MATE = 10000,

    TB_LOSS_SCORE = -9900,
    TB_WIN_SCORE = 9900,

    EVAL_MIN = -9800,
    EVAL_MAX = 9800,

    DRAW = 0,
};

constexpr int GetBitCount(uint64_t bb)
{
#if defined(__GNUG__) && defined(USE_POPCNT)
    return __builtin_popcountll(bb);
#else
    //https://www.chessprogramming.org/Population_Count
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

//std::abs is not constexpr (this is being added to the standard soon)
//for now, define my own
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

constexpr uint64_t EMPTY = 0;
constexpr uint64_t UNIVERSE = 0xffffffffffffffff;

namespace Detail //so these don't polute the global scope
{
//Not my code, slightly modified
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

constexpr auto RankBB = []() {
    std::array<uint64_t, N_RANKS> ret {};
    for (Rank i = RANK_1; i <= RANK_8; ++i)
        ret[i] = 0xffULL << (8 * i);
    return ret;
}();

constexpr auto FileBB = []() {
    std::array<uint64_t, N_FILES> ret {};
    for (File i = FILE_A; i <= FILE_H; ++i)
        ret[i] = 0x101010101010101 << i;
    return ret;
}();

constexpr auto SquareBB = []() {
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = 1ULL << i;
    return ret;
}();

constexpr auto DiagonalBB = []() {
    std::array<uint64_t, N_DIAGONALS> ret { 0x100000000000000 };
    for (Diagonal i = DIAG_A7B8; i <= DIAG_H1H1; ++i)
        if (i > N_DIAGONALS / 2)
            ret[i] = (ret[i - 1] >> 8);
        else
            ret[i] = (ret[i - 1] << 1) | (ret[i - 1] >> 8);
    return ret;
}();

constexpr auto AntiDiagonalBB = []() {
    std::array<uint64_t, N_ANTI_DIAGONALS> ret { 0x8000000000000000 };
    for (AntiDiagonal i = DIAG_G8H7; i <= DIAG_A1A1; ++i)
        if (i > N_ANTI_DIAGONALS / 2)
            ret[i] = (ret[i - 1] >> 8);
        else
            ret[i] = (ret[i - 1] >> 1) | (ret[i - 1] >> 8);
    return ret;
}();

constexpr auto betweenArray = []() {
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

constexpr auto KnightAttacks = []() {
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if ((AbsRankDiff(i, j) == 1 && AbsFileDiff(i, j) == 2) || (AbsRankDiff(i, j) == 2 && AbsFileDiff(i, j) == 1))
                ret[i] |= SquareBB[j];
    return ret;
}();

constexpr auto RookAttacks = []() {
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = (RankBB[GetRank(i)] | FileBB[GetFile(i)]) ^ SquareBB[i];
    return ret;
}();

constexpr auto BishopAttacks = []() {
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        ret[i] = (DiagonalBB[GetDiagonal(i)] | AntiDiagonalBB[GetAntiDiagonal(i)]) ^ SquareBB[i];
    return ret;
}();

constexpr auto KingAttacks = []() {
    std::array<uint64_t, N_SQUARES> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if (i != j && AbsFileDiff(i, j) <= 1 && AbsRankDiff(i, j) <= 1)
                ret[i] |= SquareBB[j];
    return ret;
}();

constexpr auto PawnAttacks = []() {
    std::array<std::array<uint64_t, N_SQUARES>, N_PLAYERS> ret {};
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if ((AbsFileDiff(i, j) == 1) && RankDiff(j, i) == 1) //either side one ahead
                ret[WHITE][i] |= SquareBB[j];
    for (Square i = SQ_A1; i <= SQ_H8; ++i)
        for (Square j = SQ_A1; j <= SQ_H8; ++j)
            if ((AbsFileDiff(i, j) == 1) && RankDiff(j, i) == -1) //either side one behind
                ret[BLACK][i] |= SquareBB[j];
    return ret;
}();

constexpr auto QueenAttacks = []() {
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

//--------------------------------------------------------------------------
// Below code adapted with permission from Terje, author of Weiss.
#ifdef USE_PEXT
// Uses the bmi2 pext instruction in place of magic bitboards
#include "x86intrin.h"

#define AttackIndex(sq, occ, table) (_pext_u64(occ, table[sq].mask))
#else
// Uses magic bitboards as explained on https://www.chessprogramming.org/Magic_Bitboards
#define AttackIndex(sq, occ, table) (((occ & table[sq].mask) * table[sq].magic) >> table[sq].shift)
#endif

namespace detail
{
struct Magic
{
    uint64_t* attacks;
    uint64_t mask;
#ifndef USE_PEXT
    uint64_t magic;
    int shift;
#endif
};

template <uint64_t size>
class MagicTable
{
public:
    MagicTable(std::array<int, 4> steps_, std::array<uint64_t, N_SQUARES> magics_)
        : steps(steps_)
        , magics(magics_)
    {
        InitSliderAttacks();
    }

    const std::array<int, 4> steps;
    const std::array<uint64_t, N_SQUARES> magics;

    uint64_t AttackMask(Square sq, uint64_t occupied = EMPTY) const { return table[sq].attacks[AttackIndex(sq, occupied, table)]; }

private:
    uint64_t& AttackMask(Square sq, uint64_t occupied = EMPTY) { return table[sq].attacks[AttackIndex(sq, occupied, table)]; }

    void InitSliderAttacks();

    std::array<Magic, N_SQUARES> table = {};
    std::array<uint64_t, size> attacks = {};
};

class BishopTable : public MagicTable<0x1480>
{
    constexpr static std::array<uint64_t, N_SQUARES> magics = {
        // clang-format off
        0xFFEDF9FD7CFCFFFFULL, 0xFC0962854A77F576ULL, 0x5822022042000000ULL, 0x2CA804A100200020ULL,
        0x0204042200000900ULL, 0x2002121024000002ULL, 0xFC0A66C64A7EF576ULL, 0x7FFDFDFCBD79FFFFULL,
        0xFC0846A64A34FFF6ULL, 0xFC087A874A3CF7F6ULL, 0x1001080204002100ULL, 0x1810080489021800ULL,
        0x0062040420010A00ULL, 0x5028043004300020ULL, 0xFC0864AE59B4FF76ULL, 0x3C0860AF4B35FF76ULL,
        0x73C01AF56CF4CFFBULL, 0x41A01CFAD64AAFFCULL, 0x040C0422080A0598ULL, 0x4228020082004050ULL,
        0x0200800400E00100ULL, 0x020B001230021040ULL, 0x7C0C028F5B34FF76ULL, 0xFC0A028E5AB4DF76ULL,
        0x0020208050A42180ULL, 0x001004804B280200ULL, 0x2048020024040010ULL, 0x0102C04004010200ULL,
        0x020408204C002010ULL, 0x02411100020080C1ULL, 0x102A008084042100ULL, 0x0941030000A09846ULL,
        0x0244100800400200ULL, 0x4000901010080696ULL, 0x0000280404180020ULL, 0x0800042008240100ULL,
        0x0220008400088020ULL, 0x04020182000904C9ULL, 0x0023010400020600ULL, 0x0041040020110302ULL,
        0xDCEFD9B54BFCC09FULL, 0xF95FFA765AFD602BULL, 0x1401210240484800ULL, 0x0022244208010080ULL,
        0x1105040104000210ULL, 0x2040088800C40081ULL, 0x43FF9A5CF4CA0C01ULL, 0x4BFFCD8E7C587601ULL,
        0xFC0FF2865334F576ULL, 0xFC0BF6CE5924F576ULL, 0x80000B0401040402ULL, 0x0020004821880A00ULL,
        0x8200002022440100ULL, 0x0009431801010068ULL, 0xC3FFB7DC36CA8C89ULL, 0xC3FF8A54F4CA2C89ULL,
        0xFFFFFCFCFD79EDFFULL, 0xFC0863FCCB147576ULL, 0x040C000022013020ULL, 0x2000104000420600ULL,
        0x0400000260142410ULL, 0x0800633408100500ULL, 0xFC087E8E4BB2F736ULL, 0x43FF9E4EF4CA2C89ULL,
        // clang-format on
    };

    constexpr static std::array<int, 4> steps = { 7, 9, -7, -9 };

public:
    BishopTable()
        : MagicTable(steps, magics) {};
};

class RookTable : public MagicTable<0x19000>
{
    constexpr static std::array<uint64_t, N_SQUARES> magics = {
        // clang-format off
        0xA180022080400230ULL, 0x0040100040022000ULL, 0x0080088020001002ULL, 0x0080080280841000ULL,
        0x4200042010460008ULL, 0x04800A0003040080ULL, 0x0400110082041008ULL, 0x008000A041000880ULL,
        0x10138001A080C010ULL, 0x0000804008200480ULL, 0x00010011012000C0ULL, 0x0022004128102200ULL,
        0x000200081201200CULL, 0x202A001048460004ULL, 0x0081000100420004ULL, 0x4000800380004500ULL,
        0x0000208002904001ULL, 0x0090004040026008ULL, 0x0208808010002001ULL, 0x2002020020704940ULL,
        0x8048010008110005ULL, 0x6820808004002200ULL, 0x0A80040008023011ULL, 0x00B1460000811044ULL,
        0x4204400080008EA0ULL, 0xB002400180200184ULL, 0x2020200080100380ULL, 0x0010080080100080ULL,
        0x2204080080800400ULL, 0x0000A40080360080ULL, 0x02040604002810B1ULL, 0x008C218600004104ULL,
        0x8180004000402000ULL, 0x488C402000401001ULL, 0x4018A00080801004ULL, 0x1230002105001008ULL,
        0x8904800800800400ULL, 0x0042000C42003810ULL, 0x008408110400B012ULL, 0x0018086182000401ULL,
        0x2240088020C28000ULL, 0x001001201040C004ULL, 0x0A02008010420020ULL, 0x0010003009010060ULL,
        0x0004008008008014ULL, 0x0080020004008080ULL, 0x0282020001008080ULL, 0x50000181204A0004ULL,
        0x48FFFE99FECFAA00ULL, 0x48FFFE99FECFAA00ULL, 0x497FFFADFF9C2E00ULL, 0x613FFFDDFFCE9200ULL,
        0xFFFFFFE9FFE7CE00ULL, 0xFFFFFFF5FFF3E600ULL, 0x0010301802830400ULL, 0x510FFFF5F63C96A0ULL,
        0xEBFFFFB9FF9FC526ULL, 0x61FFFEDDFEEDAEAEULL, 0x53BFFFEDFFDEB1A2ULL, 0x127FFFB9FFDFB5F6ULL,
        0x411FFFDDFFDBF4D6ULL, 0x0801000804000603ULL, 0x0003FFEF27EEBE74ULL, 0x7645FFFECBFEA79EULL,
        // clang-format on
    };

    constexpr static std::array<int, 4> steps = { 8, 1, -8, -1 };

public:
    RookTable()
        : MagicTable(steps, magics) {};
};

constexpr uint64_t LandingSquareBB(const int sq, const int step)
{
    int to = sq + step;

    if (to < SQ_A1 || to > SQ_H8 || std::abs(sq % 8 - to % 8) >= 2 || std::abs(sq / 8 - to / 8) >= 2)
        return EMPTY;

    return SquareBB[to];
}

// Helper function that makes a slider attack bitboard
constexpr uint64_t MakeSliderAttackBB(Square sq, uint64_t occupied, const std::array<int, 4>& steps)
{
    uint64_t attacks = 0;

    for (int dir = 0; dir < 4; dir++)
    {

        int s = sq;
        while (!(occupied & SquareBB[s]) && LandingSquareBB(s, steps[dir]))
            attacks |= SquareBB[(s += steps[dir])];
    }

    return attacks;
}

template <uint64_t size>
void MagicTable<size>::InitSliderAttacks()
{
    uint64_t* attack = attacks.data();

    for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
    {
        table[sq].attacks = attack;

        // Construct the mask
        uint64_t edges = ((RankBB[RANK_1] | RankBB[RANK_8]) & ~RankBB[GetRank(sq)])
            | ((FileBB[FILE_A] | FileBB[FILE_H]) & ~FileBB[GetFile(sq)]);

        table[sq].mask = MakeSliderAttackBB(sq, 0, steps) & ~edges;

#ifndef USE_PEXT
        table[sq].magic = magics[sq];
        table[sq].shift = 64 - GetBitCount(table[sq].mask);
#endif

        uint64_t occupied = 0;
        do
        {
            AttackMask(sq, occupied) = MakeSliderAttackBB(sq, occupied, steps);
            occupied = (occupied - table[sq].mask) & table[sq].mask; // Carry rippler
            attack++;
        } while (occupied);
    }
}
}

const detail::BishopTable bishopTable;
const detail::RookTable rookTable;

//--------------------------------------------------------------------------
