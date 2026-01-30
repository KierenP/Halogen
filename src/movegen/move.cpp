#include "movegen/move.h"

#include <cassert>
#include <iostream>

constexpr int CAPTURE_MASK = 1 << 14; // 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0
constexpr int PROMOTION_MASK = 1 << 15; // 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
constexpr int FROM_MASK = 0b111111; // 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1
constexpr int TO_MASK = 0b111111 << 6; // 0 0 0 0 1 1 1 1 1 1 0 0 0 0 0 0
constexpr int FLAG_MASK = 0b1111 << 12; // 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0

Move::Move(Square from, Square to, MoveFlag flag) noexcept
    : data(from | (to << 6) | (flag << 12))
{
    assert(from < 64);
    assert(to < 64);
}

Move::Move(uint16_t data_) noexcept
    : data(data_)
{
}

Square Move::from() const noexcept
{
    return static_cast<Square>(data & FROM_MASK);
}

Square Move::to() const noexcept
{
    return static_cast<Square>((data & TO_MASK) >> 6);
}

MoveFlag Move::flag() const noexcept
{
    return static_cast<MoveFlag>((data & FLAG_MASK) >> 12);
}

bool Move::is_promotion() const noexcept
{
    return ((data & PROMOTION_MASK) != 0);
}

bool Move::is_capture() const noexcept
{
    return ((data & CAPTURE_MASK) != 0);
}

bool Move::is_castle() const noexcept
{
    return flag() == A_SIDE_CASTLE || flag() == H_SIDE_CASTLE;
}

std::ostream& operator<<(std::ostream& os, Move m)
{
    char buffer[6] = {};
    Square from = m.from();
    Square to = m.to();

    if (m.flag() == A_SIDE_CASTLE)
    {
        to = get_square(FILE_C, enum_to<Rank>(to));
    }
    else if (m.flag() == H_SIDE_CASTLE)
    {
        to = get_square(FILE_G, enum_to<Rank>(to));
    }

    buffer[0] = 'a' + enum_to<File>(from);
    buffer[1] = '1' + enum_to<Rank>(from);
    buffer[2] = 'a' + enum_to<File>(to);
    buffer[3] = '1' + enum_to<Rank>(to);

    if (m.is_promotion())
    {
        if (m.flag() == KNIGHT_PROMOTION || m.flag() == KNIGHT_PROMOTION_CAPTURE)
            buffer[4] = 'n';
        else if (m.flag() == BISHOP_PROMOTION || m.flag() == BISHOP_PROMOTION_CAPTURE)
            buffer[4] = 'b';
        else if (m.flag() == QUEEN_PROMOTION || m.flag() == QUEEN_PROMOTION_CAPTURE)
            buffer[4] = 'q';
        else if (m.flag() == ROOK_PROMOTION || m.flag() == ROOK_PROMOTION_CAPTURE)
            buffer[4] = 'r';
    }

    return os << buffer;
}

std::ostream& operator<<(std::ostream& os, format_chess960 f)
{
    const auto& m = f.m;
    char buffer[6] = {};
    Square from = m.from();
    Square to = m.to();

    buffer[0] = 'a' + enum_to<File>(from);
    buffer[1] = '1' + enum_to<Rank>(from);
    buffer[2] = 'a' + enum_to<File>(to);
    buffer[3] = '1' + enum_to<Rank>(to);

    if (f.m.is_promotion())
    {
        if (m.flag() == KNIGHT_PROMOTION || m.flag() == KNIGHT_PROMOTION_CAPTURE)
            buffer[4] = 'n';
        else if (m.flag() == BISHOP_PROMOTION || m.flag() == BISHOP_PROMOTION_CAPTURE)
            buffer[4] = 'b';
        else if (m.flag() == QUEEN_PROMOTION || m.flag() == QUEEN_PROMOTION_CAPTURE)
            buffer[4] = 'q';
        else if (m.flag() == ROOK_PROMOTION || m.flag() == ROOK_PROMOTION_CAPTURE)
            buffer[4] = 'r';
    }

    return os << buffer;
}
