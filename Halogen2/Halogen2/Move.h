#pragma once
#include <iostream>
#include <assert.h>

enum MoveFlag
{
	UNINITIALIZED,
	QUIET,							
	PAWN_DOUBLE_MOVE,				
	KING_CASTLE,					
	QUEEN_CASTLE,					
	CAPTURE,						
	EN_PASSANT,						
	KNIGHT_PROMOTION = 8,	//no real reason, if you change this also change IsPromotion and IsCapture!		
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
	Move();
	Move(unsigned int from, unsigned int to, unsigned int flag);
	~Move();

	unsigned int GetFrom() const;
	unsigned int GetTo() const;
	unsigned int GetFlag() const;

	bool IsPromotion() const;
	bool IsCapture() const;

	void Print() const;

	bool operator==(const Move& rhs) const;
	
	int SEE;

private:

	void SetFrom(unsigned int from);
	void SetTo(unsigned int to);
	void SetFlag(unsigned int flag);

	char m_from;
	char m_to;
	char m_flag;
};


