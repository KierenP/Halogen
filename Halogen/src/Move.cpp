#include "Move.h"

const unsigned int CAPTURE_MASK = 1 << 14;		// 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0
const unsigned int PROMOTION_MASK = 1 << 15;	// 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
const unsigned int FROM_MASK = 0b111111;		// 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1
const unsigned int TO_MASK = 0b111111 << 6;		// 0 0 0 0 1 1 1 1 1 1 0 0 0 0 0 0
const unsigned int FLAG_MASK = 0b1111 << 12;	// 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 

Move::Move()
{
	SetFrom(-1);
	SetTo(-1);
	SetFlag(UNINITIALIZED);
}

Move::Move(unsigned int from, unsigned int to, unsigned int flag)
{
	assert(from < 64);
	assert(to < 64);
	assert(flag < 16);

	SetFrom(from);
	SetTo(to);
	SetFlag(flag);
}

Move::~Move()
{
}

unsigned int Move::GetFrom() const
{
	return m_from;
}

unsigned int Move::GetTo() const
{
	return m_to;
}

unsigned int Move::GetFlag() const
{
	return m_flag;
}

bool Move::IsPromotion() const
{
	if (GetFlag() >= 8)
		return true;
	return false;
}

bool Move::IsCapture() const
{
	if (m_flag == CAPTURE || m_flag == EN_PASSANT || m_flag >= 12)
		return true;
	return false;
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
	if (m_from == rhs.GetFrom() && m_to == rhs.GetTo() && m_flag == rhs.GetFlag())
		return true;
	return false;
}

void Move::SetFrom(unsigned int from)
{
	m_from = from;
}

void Move::SetTo(unsigned int to)
{
	m_to = to;
}

void Move::SetFlag(unsigned int flag)
{
	m_flag = flag;
}
