#pragma once
#include "Position.h"
#include "EvalNet.h"
#include "MoveList.h"

void LegalMoves(Position& position, ExtendedMoveList& moves);
void QuiescenceMoves(Position& position, ExtendedMoveList& moves);
void QuietMoves(Position& position, ExtendedMoveList& moves);

void LegalMoves(Position& position, BasicMoveList& moves);
void QuiescenceMoves(Position& position, BasicMoveList& moves);
void QuietMoves(Position& position, BasicMoveList& moves);

bool IsInCheck(const Position& position, Players colour);
bool IsInCheck(const Position& position);

bool MoveIsLegal(Position& position, const Move& move);

//Below code adapted with permission from Terje, author of Weiss.
//--------------------------------------------------------------------------

// Returns the attack bitboard for a piece of piecetype on square sq
template <PieceTypes pieceType> inline
uint64_t AttackBB(Square sq, uint64_t occupied = EMPTY)
{
	switch (pieceType)
	{
	case KNIGHT:	return KnightAttacks[sq];
	case KING:		return KingAttacks[sq];
	case BISHOP:	return bishopTable.AttackMask(sq, occupied);
	case ROOK:		return rookTable.AttackMask(sq, occupied);
	case QUEEN:		return AttackBB<ROOK>(sq, occupied) | AttackBB<BISHOP>(sq, occupied);
	default:		throw std::invalid_argument("piecetype is argument is invalid");
	}
}

//--------------------------------------------------------------------------