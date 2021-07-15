#pragma once
#include "BitBoardDefine.h"
#include <iostream>
#include <assert.h>
#include <sstream>

enum MoveFlag
{
	QUIET,							
	PAWN_DOUBLE_MOVE,				
	KING_CASTLE,					
	QUEEN_CASTLE,					
	CAPTURE,						
	EN_PASSANT = 5,				

	DONT_USE_1 = 6,
	DONT_USE_2 = 7,

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
	Move() = default;
	Move(Square from, Square to, MoveFlag flag);

	Square GetFrom() const;
	Square GetTo() const;
	MoveFlag GetFlag() const;

	bool IsPromotion() const;
	bool IsCapture() const;

	void Print(std::stringstream& ss) const;
	void Print() const;

	constexpr bool operator==(const Move& rhs) const
	{
		return (data == rhs.data);
	}

	constexpr bool operator!=(const Move& rhs) const
	{
		return (data != rhs.data);
	}

	static const Move Uninitialized;

private:
	void SetFrom(Square from);
	void SetTo(Square to);
	void SetFlag(MoveFlag flag);

	//6 bits for 'from square', 6 bits for 'to square' and 4 bits for the 'move flag'
	uint16_t data;
};

// Why do we mandate that Move is trivial and allow the default
// constructor to have member 'data' be uninitialized? Because
// we need to allocate arrays of Move objects on the stack
// millions of times per second and making Move trivial makes
// the allocation virtually free.

static_assert(std::is_trivial_v<Move>);

inline const Move Move::Uninitialized = Move(static_cast<Square>(0), static_cast<Square>(0), static_cast<MoveFlag>(0));

