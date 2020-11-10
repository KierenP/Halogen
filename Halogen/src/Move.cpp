#include "Move.h"

const unsigned int CAPTURE_MASK = 1 << 14;		// 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0
const unsigned int PROMOTION_MASK = 1 << 15;	// 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
const unsigned int FROM_MASK = 0b111111;		// 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1
const unsigned int TO_MASK = 0b111111 << 6;		// 0 0 0 0 1 1 1 1 1 1 0 0 0 0 0 0
const unsigned int FLAG_MASK = 0b1111 << 12;	// 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 

Move::Move()
{
	data = 0;
	orderScore = 0;
}

Move::Move(unsigned int from, unsigned int to, unsigned int flag)
{
	assert(from < 64);
	assert(to < 64);
	assert(flag < 16);

	data = 0;
	orderScore = 0;
	SetFrom(from);
	SetTo(to);
	SetFlag(flag);
}

Move::Move(unsigned short bits)
{
	data = bits;
}

Move::~Move()
{
}

unsigned int Move::GetFrom() const
{
	return (data & FROM_MASK);
}

unsigned int Move::GetTo() const
{
	return (data & TO_MASK) >> 6;
}

unsigned int Move::GetFlag() const
{
	return (data & FLAG_MASK) >> 12;
}

bool Move::IsPromotion() const
{
	return ((data & PROMOTION_MASK) != 0);
}

bool Move::IsCapture() const
{
	return ((data & CAPTURE_MASK) != 0);
}

void Move::Print() const
{
	unsigned int prev = GetFrom();
	unsigned int current = GetTo();

	std::cout << (char)(prev % 8 + 97) << (prev / 8) + 1 << (char)(current % 8 + 97) << (current / 8) + 1;	//+1 to make it from 1-8 and not 0-7, 97 is ascii for 'a'

	if (IsPromotion())
	{
		if (GetFlag() == KNIGHT_PROMOTION || GetFlag() == KNIGHT_PROMOTION_CAPTURE)
			std::cout << "n";
		if (GetFlag() == BISHOP_PROMOTION || GetFlag() == BISHOP_PROMOTION_CAPTURE)
			std::cout << "b";
		if (GetFlag() == QUEEN_PROMOTION || GetFlag() == QUEEN_PROMOTION_CAPTURE)
			std::cout << "q";
		if (GetFlag() == ROOK_PROMOTION || GetFlag() == ROOK_PROMOTION_CAPTURE)
			std::cout << "r";
	}
}

bool Move::operator==(const Move & rhs) const
{
	return (data == rhs.data);
}

bool Move::IsUninitialized() const
{
	return (data == 0);
}

void Move::Reset()
{
	data = 0;
}

void Move::SetFrom(unsigned int from)
{
	data &= ~FROM_MASK;
	data |= from;
}

void Move::SetTo(unsigned int to)
{
	data &= ~TO_MASK;
	data |= to << 6;
}

void Move::SetFlag(unsigned int flag)
{
	data &= ~FLAG_MASK;
	data |= flag << 12;
}
