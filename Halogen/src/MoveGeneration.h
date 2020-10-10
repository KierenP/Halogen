#pragma once
#include "Position.h"
#include "EvalNet.h"

void LegalMoves(Position& position, std::vector<Move>& moves);
void QuiescenceMoves(Position& position, std::vector<Move>& moves);

bool IsSquareThreatened(const Position & position, unsigned int square, bool colour);		//will tell you if the king WOULD be threatened on that square. Useful for finding defended / threatening pieces
bool IsInCheck(const Position& position, bool colour);	
bool IsInCheck(const Position& position);
uint64_t GetThreats(const Position& position, unsigned int square, bool colour);	//colour is of the attacked piece! So to get the black threats of a white piece pass colour = WHITE!

Move GetSmallestAttackerMove(const Position& position, unsigned int square, bool colour);

bool MoveIsLegal(Position& position, Move& move);

