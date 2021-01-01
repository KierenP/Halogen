#pragma once
#include "SearchData.h"
#include "MoveGeneration.h"

enum class Stage 
{
	TT_MOVE,
	GEN_OTHERS, GIVE_OTHERS
};

class MoveGenerator
{
public:
	MoveGenerator(Position& Position, int DistanceFromRoot, const SearchData& Locals, bool Quiessence);
	bool Next(Move& move);	//returns false if no more legal moves

private:
	void OrderMoves(std::vector<Move>& moves);
	
	//Data needed for use in ordering or generating moves
	Position& position;
	int distanceFromRoot;
	const SearchData& locals;
	bool quiescence;

	//Data uses for keeping track of internal values
	Stage stage;
	std::vector<Move> legalMoves;
	std::vector<Move>::iterator current;
};

Move GetHashMove(const Position& position, int depthRemaining, int distanceFromRoot);
Move GetHashMove(const Position& position, int distanceFromRoot);

int see(Position& position, Square square, Players side);
int seeCapture(Position& position, const Move& move); //Don't send this an en passant move!

