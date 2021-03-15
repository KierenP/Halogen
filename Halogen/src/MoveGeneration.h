#pragma once
#include "Position.h"
#include "EvalNet.h"

// normally we would just generate a vector of Move, but later
// we need to convert that vector to a vector of ExtendedMove. 
// This is expensive, and its actually better to just create
// the ExtendedMove vector from the start.
struct ExtendedMove
{
	ExtendedMove(const Move _move, const int _score = 0, const short int _SEE = 0) : move(_move), score(_score), SEE(_SEE) {}
	ExtendedMove(Square from, Square to, MoveFlag flag) 
		: move(CreateMove(from, to, flag)), score(0), SEE(0) {}

	bool operator<(const ExtendedMove& rhs) const { return score < rhs.score; };
	bool operator>(const ExtendedMove& rhs) const { return score > rhs.score; };

	Move move = Move::invalid;
	int16_t score;
	int16_t SEE;
};

void LegalMoves(Position& position, std::vector<ExtendedMove>& moves);
void QuiescenceMoves(Position& position, std::vector<ExtendedMove>& moves);
void QuietMoves(Position& position, std::vector<ExtendedMove>& moves);

bool IsSquareThreatened(const Position & position, Square square, Players colour);		//will tell you if the king WOULD be threatened on that square. Useful for finding defended / threatening pieces
bool IsInCheck(const Position& position, Players colour);
bool IsInCheck(const Position& position);
uint64_t GetThreats(const Position& position, Square square, Players colour);	//colour is of the attacked piece! So to get the black threats of a white piece pass colour = WHITE!

Move GetSmallestAttackerMove(const Position& position, Square square, Players colour);

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
	case BISHOP:	return BishopTable[sq].attacks[AttackIndex(sq, occupied, BishopTable)];
	case ROOK:		return RookTable[sq].attacks[AttackIndex(sq, occupied, RookTable)];
	case QUEEN:		return AttackBB<ROOK>(sq, occupied) | AttackBB<BISHOP>(sq, occupied);
	default:		throw std::invalid_argument("piecetype is argument is invalid");
	}
}

//--------------------------------------------------------------------------