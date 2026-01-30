#pragma once

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

// TODO: many of these enums should be replaced with enum class

enum Square : int8_t
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

constexpr Square& operator++(Square& sq) noexcept
{
    assert(sq < N_SQUARES);
    sq = static_cast<Square>(sq + 1);
    return sq;
}

enum Side : int8_t
{
    BLACK,
    WHITE,

    N_SIDES
};

constexpr Side operator!(const Side& val) noexcept
{
    return val == WHITE ? BLACK : WHITE;
}

enum Piece : int8_t
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

enum PieceType : int8_t
{
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,

    N_PIECE_TYPES
};

enum Rank : int8_t
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

constexpr Rank mirror(Rank rank) noexcept
{
    assert(rank < N_RANKS);
    return static_cast<Rank>(RANK_8 - rank);
}

constexpr Rank& operator++(Rank& rank) noexcept
{
    assert(rank < N_RANKS);
    rank = static_cast<Rank>(rank + 1);
    return rank;
}

enum File : int8_t
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

constexpr File& operator++(File& file) noexcept
{
    assert(file < N_FILES);
    file = static_cast<File>(file + 1);
    return file;
}

enum Diagonal : int8_t
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

constexpr Diagonal& operator++(Diagonal& diagonal) noexcept
{
    assert(diagonal < N_DIAGONALS);
    diagonal = static_cast<Diagonal>(diagonal + 1);
    return diagonal;
}

enum AntiDiagonal : int8_t
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

constexpr AntiDiagonal& operator++(AntiDiagonal& antidiagonal) noexcept
{
    assert(antidiagonal < N_ANTI_DIAGONALS);
    antidiagonal = static_cast<AntiDiagonal>(antidiagonal + 1);
    return antidiagonal;
}

// clang-format off
static constexpr File square_to_file[N_SQUARES] = {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
};

static constexpr Rank square_to_rank[N_SQUARES] = {
    RANK_1, RANK_1, RANK_1, RANK_1, RANK_1, RANK_1, RANK_1, RANK_1,
    RANK_2, RANK_2, RANK_2, RANK_2, RANK_2, RANK_2, RANK_2, RANK_2,
    RANK_3, RANK_3, RANK_3, RANK_3, RANK_3, RANK_3, RANK_3, RANK_3,
    RANK_4, RANK_4, RANK_4, RANK_4, RANK_4, RANK_4, RANK_4, RANK_4,
    RANK_5, RANK_5, RANK_5, RANK_5, RANK_5, RANK_5, RANK_5, RANK_5,
    RANK_6, RANK_6, RANK_6, RANK_6, RANK_6, RANK_6, RANK_6, RANK_6,
    RANK_7, RANK_7, RANK_7, RANK_7, RANK_7, RANK_7, RANK_7, RANK_7,
    RANK_8, RANK_8, RANK_8, RANK_8, RANK_8, RANK_8, RANK_8, RANK_8,
};

static constexpr Diagonal square_to_diagonal[N_SQUARES] = {
    DIAG_A1H8, DIAG_B1H7, DIAG_C1H6, DIAG_D1H5, DIAG_E1H4, DIAG_F1H3, DIAG_G1H2, DIAG_H1H1,
    DIAG_A2G8, DIAG_A1H8, DIAG_B1H7, DIAG_C1H6, DIAG_D1H5, DIAG_E1H4, DIAG_F1H3, DIAG_G1H2,
    DIAG_A3F8, DIAG_A2G8, DIAG_A1H8, DIAG_B1H7, DIAG_C1H6, DIAG_D1H5, DIAG_E1H4, DIAG_F1H3,
    DIAG_A4E8, DIAG_A3F8, DIAG_A2G8, DIAG_A1H8, DIAG_B1H7, DIAG_C1H6, DIAG_D1H5, DIAG_E1H4,
    DIAG_A5D8, DIAG_A4E8, DIAG_A3F8, DIAG_A2G8, DIAG_A1H8, DIAG_B1H7, DIAG_C1H6, DIAG_D1H5,
    DIAG_A6C8, DIAG_A5D8, DIAG_A4E8, DIAG_A3F8, DIAG_A2G8, DIAG_A1H8, DIAG_B1H7, DIAG_C1H6,
    DIAG_A7B8, DIAG_A6C8, DIAG_A5D8, DIAG_A4E8, DIAG_A3F8, DIAG_A2G8, DIAG_A1H8, DIAG_B1H7,
    DIAG_A8A8, DIAG_A7B8, DIAG_A6C8, DIAG_A5D8, DIAG_A4E8, DIAG_A3F8, DIAG_A2G8, DIAG_A1H8
};

static constexpr AntiDiagonal square_to_antidiagonal[N_SQUARES] = {
    DIAG_A1A1, DIAG_A2B1, DIAG_A3C1, DIAG_A4D1, DIAG_A5E1, DIAG_A6F1, DIAG_A7G1, DIAG_A8H1,
    DIAG_A2B1, DIAG_A3C1, DIAG_A4D1, DIAG_A5E1, DIAG_A6F1, DIAG_A7G1, DIAG_A8H1, DIAG_B8H2,
    DIAG_A3C1, DIAG_A4D1, DIAG_A5E1, DIAG_A6F1, DIAG_A7G1, DIAG_A8H1, DIAG_B8H2, DIAG_C8H3,
    DIAG_A4D1, DIAG_A5E1, DIAG_A6F1, DIAG_A7G1, DIAG_A8H1, DIAG_B8H2, DIAG_C8H3, DIAG_D8H4,
    DIAG_A5E1, DIAG_A6F1, DIAG_A7G1, DIAG_A8H1, DIAG_B8H2, DIAG_C8H3, DIAG_D8H4, DIAG_E8H5,
    DIAG_A6F1, DIAG_A7G1, DIAG_A8H1, DIAG_B8H2, DIAG_C8H3, DIAG_D8H4, DIAG_E8H5, DIAG_F8H6,
    DIAG_A7G1, DIAG_A8H1, DIAG_B8H2, DIAG_C8H3, DIAG_D8H4, DIAG_E8H5, DIAG_F8H6, DIAG_G8H7,
    DIAG_A8H1, DIAG_B8H2, DIAG_C8H3, DIAG_D8H4, DIAG_E8H5, DIAG_F8H6, DIAG_G8H7, DIAG_H8H8
};

static constexpr Side piece_to_side[N_PIECES] = {
    BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE
};

static constexpr PieceType piece_to_type[N_PIECES] = {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};
// clang-format on

template <typename T, typename U>
constexpr T enum_to(U u) noexcept;

template <>
constexpr Side enum_to(Piece piece) noexcept
{
    assert(piece < N_PIECES);
    return piece_to_side[piece];
}

template <>
constexpr PieceType enum_to(Piece piece) noexcept
{
    assert(piece < N_PIECES);
    return piece_to_type[piece];
}

template <>
constexpr File enum_to(Square square) noexcept
{
    assert(square < N_SQUARES);
    return square_to_file[square];
}

template <>
constexpr Rank enum_to(Square square) noexcept
{
    assert(square < N_SQUARES);
    return square_to_rank[square];
}

template <>
constexpr Diagonal enum_to(Square square) noexcept
{
    assert(square < N_SQUARES);
    return square_to_diagonal[square];
}

template <>
constexpr AntiDiagonal enum_to(Square square) noexcept
{
    assert(square < N_SQUARES);
    return square_to_antidiagonal[square];
}

constexpr int rank_diff(Square sq1, Square sq2) noexcept
{
    assert(sq1 < N_SQUARES && sq2 < N_SQUARES);
    return static_cast<int>(enum_to<Rank>(sq1)) - static_cast<int>(enum_to<Rank>(sq2));
}

constexpr int file_diff(Square sq1, Square sq2) noexcept
{
    assert(sq1 < N_SQUARES && sq2 < N_SQUARES);
    return static_cast<int>(enum_to<File>(sq1)) - static_cast<int>(enum_to<File>(sq2));
}

// std::abs is not constexpr (this is being added to the standard soon)
// for now, define my own
template <class T, std::enable_if_t<std::is_arithmetic_v<T>>...>
constexpr auto abs_constexpr(T const& x) noexcept
{
    return x < 0 ? -x : x;
}

constexpr int abs_rank_diff(Square sq1, Square sq2) noexcept
{
    assert(sq1 < N_SQUARES);
    assert(sq2 < N_SQUARES);

    return abs_constexpr(static_cast<int>(enum_to<Rank>(sq1)) - static_cast<int>(enum_to<Rank>(sq2)));
}

constexpr int abs_file_diff(Square sq1, Square sq2) noexcept
{
    assert(sq1 < N_SQUARES);
    assert(sq2 < N_SQUARES);

    return abs_constexpr(static_cast<int>(enum_to<File>(sq1)) - static_cast<int>(enum_to<File>(sq2)));
}

constexpr Piece get_piece(PieceType type, Side colour) noexcept
{
    assert(type < N_PIECE_TYPES);
    assert(colour < N_SIDES);

    return static_cast<Piece>((int)type + (int)N_PIECE_TYPES * (int)colour);
}

constexpr Square get_square(File file, Rank rank) noexcept
{
    assert(file < N_FILES);
    assert(rank < N_RANKS);

    return static_cast<Square>((int)rank * 8 + (int)file);
}
