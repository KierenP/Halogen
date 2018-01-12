#pragma once
#include <iostream>

enum MoveFlag
{
	/*
	3rd bit always signifies a capture
	4th bit always signifies a promotion
	*/

	QUIET,							//0		0 0 0 0
	PAWN_DOUBLE_MOVE,				//1		0 0 0 1
	KING_CASTLE,					//2		0 0 1 0
	QUEEN_CASTLE,					//3		0 0 1 1
	CAPTURE,						//4		0 1 0 0
	EN_PASSANT,						//5		0 1 0 1
	KNIGHT_PROMOTION = 8,			//8		1 0 0 0
	BISHOP_PROMOTION,				//9		1 0 0 1
	ROOK_PROMOTION,					//10	1 0 1 0
	QUEEN_PROMOTION,				//11	1 0 1 1
	KNIGHT_PROMOTION_CAPTURE,		//12	1 1 0 0
	BISHOP_PROMOTION_CAPTURE,		//13	1 1 0 1	
	ROOK_PROMOTION_CAPTURE,			//14	1 1 1 0
	QUEEN_PROMOTION_CAPTURE			//15	1 1 1 1
};

class Move
{
public:
	explicit Move();
	explicit Move(unsigned int from, unsigned int to, unsigned int flag);
	~Move();

	void SetFrom(unsigned int from);
	void SetTo(unsigned int to);
	void SetFlag(unsigned int flag);

	unsigned int GetFrom();
	unsigned int GetTo();
	unsigned int GetFlag();

	bool IsPromotion();
	bool IsCapture();

	void Print();

private:

	char m_From;
	char m_To;
	char m_Flag;
};


