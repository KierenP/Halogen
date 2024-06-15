#include "Move.h"

#include <assert.h>
#include <iostream>

constexpr int CAPTURE_MASK = 1 << 14; // 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0
constexpr int PROMOTION_MASK = 1 << 15; // 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
constexpr int FROM_MASK = 0b111111; // 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1
constexpr int TO_MASK = 0b111111 << 6; // 0 0 0 0 1 1 1 1 1 1 0 0 0 0 0 0
constexpr int FLAG_MASK = 0b1111 << 12; // 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0

Move::Move(Square from, Square to, MoveFlag flag)
    : data(0)
{
    assert(from < 64);
    assert(to < 64);

    SetFrom(from);
    SetTo(to);
    SetFlag(flag);
}

Move::Move(uint16_t data_)
    : data(data_)
{
}

Square Move::GetFrom() const
{
    return static_cast<Square>(data & FROM_MASK);
}

Square Move::GetTo() const
{
    return static_cast<Square>((data & TO_MASK) >> 6);
}

MoveFlag Move::GetFlag() const
{
    return static_cast<MoveFlag>((data & FLAG_MASK) >> 12);
}

bool Move::IsPromotion() const
{
    return ((data & PROMOTION_MASK) != 0);
}

bool Move::IsCapture() const
{
    return ((data & CAPTURE_MASK) != 0);
}

bool Move::IsCastle() const
{
    return GetFlag() == A_SIDE_CASTLE || GetFlag() == H_SIDE_CASTLE;
}

std::ostream& operator<<(std::ostream& os, Move m)
{
    char buffer[6] = {};
    Square from = m.GetFrom();
    Square to = m.GetTo();

    buffer[0] = 'a' + GetFile(from);
    buffer[1] = '1' + GetRank(from);
    buffer[2] = 'a' + GetFile(to);
    buffer[3] = '1' + GetRank(to);

    if (m.IsPromotion())
    {
        if (m.GetFlag() == KNIGHT_PROMOTION || m.GetFlag() == KNIGHT_PROMOTION_CAPTURE)
            buffer[4] = 'n';
        else if (m.GetFlag() == BISHOP_PROMOTION || m.GetFlag() == BISHOP_PROMOTION_CAPTURE)
            buffer[4] = 'b';
        else if (m.GetFlag() == QUEEN_PROMOTION || m.GetFlag() == QUEEN_PROMOTION_CAPTURE)
            buffer[4] = 'q';
        else if (m.GetFlag() == ROOK_PROMOTION || m.GetFlag() == ROOK_PROMOTION_CAPTURE)
            buffer[4] = 'r';
    }

    return os << buffer;
}

void Move::SetFrom(Square from)
{
    data &= ~FROM_MASK;
    data |= from;
}

void Move::SetTo(Square to)
{
    data &= ~TO_MASK;
    data |= to << 6;
}

void Move::SetFlag(MoveFlag flag)
{
    data &= ~FLAG_MASK;
    data |= flag << 12;
}
