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

std::string Move::to_string_960(Players stm, uint64_t castle_sq) const
{
    Square from = GetFrom();
    Square to = GetTo();

    // in chess960, castle moves are encoded as king takes rook
    if (GetFlag() == A_SIDE_CASTLE && stm == WHITE)
    {
        to = LSB(castle_sq & RankBB[RANK_1]);
    }
    if (GetFlag() == H_SIDE_CASTLE && stm == WHITE)
    {
        to = MSB(castle_sq & RankBB[RANK_1]);
    }
    if (GetFlag() == A_SIDE_CASTLE && stm == BLACK)
    {
        to = LSB(castle_sq & RankBB[RANK_8]);
    }
    if (GetFlag() == H_SIDE_CASTLE && stm == BLACK)
    {
        to = MSB(castle_sq & RankBB[RANK_8]);
    }

    std::string str;
    str.reserve(5);

    str += GetFile(from) + 'a';
    str += GetRank(from) + '1';
    str += GetFile(to) + 'a';
    str += GetRank(to) + '1';

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

std::string Move::to_string() const
{
    Square from = GetFrom();
    Square to = GetTo();

    std::string str;
    str.reserve(5);

    str += GetFile(from) + 'a';
    str += GetRank(from) + '1';
    str += GetFile(to) + 'a';
    str += GetRank(to) + '1';

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

void Move::Print960(Players stm, uint64_t castle_sq) const
{
    std::cout << to_string_960(stm, castle_sq);
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
