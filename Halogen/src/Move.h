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

enum Move : uint16_t {
	invalid
};

#define CreateMove(from, to, flag) (Move(((from)) | ((to) << 6) | ((flag) << 12)))

constexpr Square GetFrom(Move move)
{
	return static_cast<Square>(move & 0b111111);
}

constexpr Square GetTo(Move move)
{
	return static_cast<Square>((move >> 6) & 0b111111);
}

constexpr MoveFlag GetFlag(Move move)
{
	return static_cast<MoveFlag>((move >> 12) & 0b1111);
}

constexpr bool isPromotion(Move move)
{
	return move & (1ull << 15);
}

constexpr bool IsCapture(Move move)
{
	return move & (1ull << 14);
}

void PrintMove(Move, std::ostream& ss);
inline void PrintMove(Move move)
{
	PrintMove(move, std::cout);
}