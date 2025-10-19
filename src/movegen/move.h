#pragma once
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <type_traits>

#include "bitboard/define.h"

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
    Move(Square from, Square to, MoveFlag flag);
    explicit Move(uint16_t data);

    Square from() const;
    Square to() const;
    MoveFlag flag() const;

    bool is_promotion() const;
    bool is_capture() const;
    bool is_castle() const;

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
    // 6 bits for 'from square', 6 bits for 'to square' and 4 bits for the 'move flag'
    uint16_t data;
};

// Why do we mandate that Move is trivial and allow the default
// constructor to have member 'data' be uninitialized? Because
// we need to allocate arrays of Move objects on the stack
// millions of times per second and making Move trivial makes
// the allocation virtually free.

static_assert(std::is_trivial_v<Move>);

inline const Move Move::Uninitialized = Move(static_cast<Square>(0), static_cast<Square>(0), static_cast<MoveFlag>(0));

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