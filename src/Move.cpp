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
    assert(flag < 16);

    SetFrom(from);
    SetTo(to);
    SetFlag(flag);
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

std::string Move::to_string() const
{
    Square prev = GetFrom();
    Square current = GetTo();

    std::string str;
    str.reserve(5);

    str += GetFile(prev) + 'a';
    str += GetRank(prev) + '1';
    str += GetFile(current) + 'a';
    str += GetRank(current) + '1';

    if (IsPromotion())
    {
        if (GetFlag() == KNIGHT_PROMOTION || GetFlag() == KNIGHT_PROMOTION_CAPTURE)
            str += "n";
        if (GetFlag() == BISHOP_PROMOTION || GetFlag() == BISHOP_PROMOTION_CAPTURE)
            str += "b";
        if (GetFlag() == QUEEN_PROMOTION || GetFlag() == QUEEN_PROMOTION_CAPTURE)
            str += "q";
        if (GetFlag() == ROOK_PROMOTION || GetFlag() == ROOK_PROMOTION_CAPTURE)
            str += "r";
    }

    return str;
}

void Move::Print() const
{
    std::cout << to_string();
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
