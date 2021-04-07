#include "Move.h"

const unsigned int CAPTURE_MASK = 1 << 14;		// 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0
const unsigned int PROMOTION_MASK = 1 << 15;	// 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
const unsigned int FROM_MASK = 0b111111;		// 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1
const unsigned int TO_MASK = 0b111111 << 6;		// 0 0 0 0 1 1 1 1 1 1 0 0 0 0 0 0
const unsigned int FLAG_MASK = 0b1111 << 12;	// 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 

Move::Move()
{
}

Move::Move(Square from, Square to, MoveFlag flag)
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

void Move::Print(std::stringstream& ss) const
{
	Square prev = GetFrom();
	Square current = GetTo();

	ss << (char)(GetFile(prev) + 'a') << GetRank(prev) + 1 << (char)(GetFile(current) + 'a') << GetRank(current) + 1;	//+1 to make it from 1-8 and not 0-7

	if (IsPromotion())
	{
		if (GetFlag() == KNIGHT_PROMOTION || GetFlag() == KNIGHT_PROMOTION_CAPTURE)
			ss << "n";
		if (GetFlag() == BISHOP_PROMOTION || GetFlag() == BISHOP_PROMOTION_CAPTURE)
			ss << "b";
		if (GetFlag() == QUEEN_PROMOTION || GetFlag() == QUEEN_PROMOTION_CAPTURE)
			ss << "q";
		if (GetFlag() == ROOK_PROMOTION || GetFlag() == ROOK_PROMOTION_CAPTURE)
			ss << "r";
	}
}

void Move::Print() const
{
	std::stringstream str;
	Print(str);
	std::cout << str.str();
}

bool Move::operator==(const Move& rhs) const
{
	return (data == rhs.data);
}

bool Move::IsUninitialized() const
{
	return (data == 0);
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
