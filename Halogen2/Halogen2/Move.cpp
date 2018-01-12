#include "Move.h"

static unsigned int CAPTURE_FLAG = 0x4;		//0 1 0 0
static unsigned int PROMOTION_FLAG = 0x8;	//1 0 0 0

Move::Move()
{
	m_From = 0;
	m_To = 0;
	m_Flag = 0;
}

Move::Move(unsigned int from, unsigned int to, unsigned int flag)
{
	m_From = from;
	m_To = to;
	m_Flag = flag;
}

Move::~Move()
{
}

void Move::SetFrom(unsigned int from)
{
	m_From = from;
}

void Move::SetTo(unsigned int to)
{
	m_To = to;
}

void Move::SetFlag(unsigned int flag)
{
	m_Flag = flag;
}

unsigned int Move::GetFrom()
{
	return m_From;
}

unsigned int Move::GetTo()
{
	return m_To;
}

unsigned int Move::GetFlag()
{
	return m_Flag;
}

bool Move::IsPromotion()
{
	if (m_Flag >= 8)
		return true;
	return false;
}

bool Move::IsCapture()
{
	if (m_Flag == CAPTURE || m_Flag == EN_PASSANT || m_Flag >= 12)
		return true;
	return false;
}

void Move::Print()
{
	unsigned int prev = GetFrom();
	unsigned int current = GetTo();

	std::cout << (char)(prev % 8 + 97) << prev / 8 + 1 << (char)(current % 8 + 97) << current / 8 + 1;	//+1 to make it from 1-8 and not 0-7, 97 is ascii for 'a'

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

	//std::cout << std::endl;
}
