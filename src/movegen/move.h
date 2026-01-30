#pragma once

#include "bitboard/enum.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <type_traits>

enum MoveFlag : int8_t
{
    QUIET,
    PAWN_DOUBLE_MOVE,
    A_SIDE_CASTLE,
    H_SIDE_CASTLE,
    CAPTURE,
    EN_PASSANT = 5,

    DONT_USE_1 = 6,
    DONT_USE_2 = 7,

    KNIGHT_PROMOTION = 8,
    BISHOP_PROMOTION,
    ROOK_PROMOTION,
    QUEEN_PROMOTION,
    KNIGHT_PROMOTION_CAPTURE,
    BISHOP_PROMOTION_CAPTURE,
    ROOK_PROMOTION_CAPTURE,
    QUEEN_PROMOTION_CAPTURE
};

class Move
{
public:
    Move() = default;
    constexpr Move(Square from, Square to, MoveFlag flag);
    constexpr explicit Move(uint16_t data);

    constexpr Square from() const;
    constexpr Square to() const;
    constexpr MoveFlag flag() const;

    constexpr bool is_promotion() const;
    constexpr bool is_capture() const;
    constexpr bool is_castle() const;

    constexpr bool operator==(const Move& rhs) const
    {
        return (data == rhs.data);
    }

    constexpr bool operator!=(const Move& rhs) const
    {
        return (data != rhs.data);
    }

    static const Move Uninitialized;

    friend std::ostream& operator<<(std::ostream& os, Move m);
    friend struct std::hash<Move>;

private:
    static constexpr int CAPTURE_MASK = 1 << 14; // 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    static constexpr int PROMOTION_MASK = 1 << 15; // 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    static constexpr int FROM_MASK = 0b111111; // 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1
    static constexpr int TO_MASK = 0b111111 << 6; // 0 0 0 0 1 1 1 1 1 1 0 0 0 0 0 0
    static constexpr int FLAG_MASK = 0b1111 << 12; // 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0

    // 6 bits for 'from square', 6 bits for 'to square' and 4 bits for the 'move flag'
    uint16_t data;
};

// Why do we mandate that Move is trivial and allow the default
// constructor to have member 'data' be uninitialized? Because
// we need to allocate arrays of Move objects on the stack
// millions of times per second and making Move trivial makes
// the allocation virtually free.

static_assert(std::is_trivial_v<Move>);

constexpr Move::Move(Square from, Square to, MoveFlag flag)
    : data(from | (to << 6) | (flag << 12))
{
}

constexpr Move::Move(uint16_t data_)
    : data(data_)
{
}

constexpr Square Move::from() const
{
    return static_cast<Square>(data & FROM_MASK);
}

constexpr Square Move::to() const
{
    return static_cast<Square>((data & TO_MASK) >> 6);
}

constexpr MoveFlag Move::flag() const
{
    return static_cast<MoveFlag>((data & FLAG_MASK) >> 12);
}

constexpr bool Move::is_promotion() const
{
    return ((data & PROMOTION_MASK) != 0);
}

constexpr bool Move::is_capture() const
{
    return ((data & CAPTURE_MASK) != 0);
}

constexpr bool Move::is_castle() const
{
    return flag() == A_SIDE_CASTLE || flag() == H_SIDE_CASTLE;
}

inline constexpr Move Move::Uninitialized = Move(static_cast<Square>(0), static_cast<Square>(0), static_cast<MoveFlag>(0));

// Compile-time verification that Move is constexpr
static_assert(Move(SQ_E2, SQ_E4, PAWN_DOUBLE_MOVE).from() == SQ_E2);
static_assert(Move(SQ_E2, SQ_E4, PAWN_DOUBLE_MOVE).to() == SQ_E4);
static_assert(Move(SQ_E2, SQ_E4, PAWN_DOUBLE_MOVE).flag() == PAWN_DOUBLE_MOVE);
static_assert(!Move(SQ_E2, SQ_E4, PAWN_DOUBLE_MOVE).is_promotion());
static_assert(!Move(SQ_E2, SQ_E4, PAWN_DOUBLE_MOVE).is_capture());
static_assert(Move(SQ_E1, SQ_H1, H_SIDE_CASTLE).is_castle());
static_assert(Move(SQ_E7, SQ_E8, QUEEN_PROMOTION).is_promotion());
static_assert(Move(SQ_E7, SQ_D8, QUEEN_PROMOTION_CAPTURE).is_capture());

struct format_chess960
{
    Move m;

    friend std::ostream& operator<<(std::ostream& os, format_chess960 f);
};

namespace std
{
template <>
struct hash<Move>
{
    std::size_t operator()(const Move& m) const
    {
        return std::hash<uint16_t>()(m.data);
    }
};
}