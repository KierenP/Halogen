#pragma once
#include "BitBoardDefine.h"
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
	Move(Square from, Square to, MoveFlag flag);
	Move(uint16_t bits);
	~Move();

	Square GetFrom() const;
	Square GetTo() const;
	MoveFlag GetFlag() const;

	bool IsPromotion() const;
	bool IsCapture() const;

	void Print() const;

	bool operator==(const Move& rhs) const;
	bool operator<(const Move& rhs) const;
	bool operator>(const Move& rhs) const;

	bool IsUninitialized() const;

	void Reset();

	int orderScore = 0;

	uint16_t GetBits() const { return data; }

private:

	void SetFrom(Square from);
	void SetTo(Square to);
	void SetFlag(MoveFlag flag);

	//6 bits for 'from square', 6 bits for 'to square' and 4 bits for the 'move flag'
	uint16_t data = 0;
};

struct MoveBits
{
	uint16_t data;
};


