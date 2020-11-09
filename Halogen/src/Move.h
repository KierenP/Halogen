#pragma once
#include <iostream>
#include <assert.h>

enum MoveFlag
{
	QUIET,							
	PAWN_DOUBLE_MOVE,				
	KING_CASTLE,					
	QUEEN_CASTLE,					
	CAPTURE,						
	EN_PASSANT,						
	KNIGHT_PROMOTION = 8,			
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
	Move(unsigned short bits);
	~Move();

	unsigned int GetFrom() const;
	unsigned int GetTo() const;
	unsigned int GetFlag() const;

	bool IsPromotion() const;
	bool IsCapture() const;

	void Print() const;

	bool operator==(const Move& rhs) const;

	bool IsUninitialized() const;

	void Reset();

	int orderScore;

	unsigned short GetBits() const { return data; }

private:

	void SetFrom(unsigned int from);
	void SetTo(unsigned int to);
	void SetFlag(unsigned int flag);

	//6 bits for 'from square', 6 bits for 'to square' and 4 bits for the 'move flag'
	unsigned short data;
};

struct MoveBits
{
	unsigned short data;
};


