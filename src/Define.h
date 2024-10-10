#pragma once

#include <cassert>
#include <cstdint>

enum class SearchResultType : uint8_t
{
    EMPTY,
    EXACT,
    LOWER_BOUND,
    UPPER_BOUND,
};

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
