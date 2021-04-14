#pragma once
#include "SearchData.h"
#include "MoveGeneration.h"

enum class Stage
{
	TT_MOVE,
	GEN_LOUD,
	GIVE_GOOD_LOUD,
	GEN_KILLER_1, GIVE_KILLER_1,
	GEN_KILLER_2, GIVE_KILLER_2,
	GIVE_BAD_LOUD,
	GEN_QUIET, GIVE_QUIET
};

class MoveGenerator
{
public:
	MoveGenerator(Position& Position, int DistanceFromRoot, const SearchData& Locals, MoveList& MoveList, bool Quiescence);
	bool Next(Move& move);	//returns false if no more legal moves
	int GetSEE() const { return (current - 1)->SEE; }

	void AdjustHistory(const Move& move, SearchData& Locals, int depthRemaining) const;

	void SkipQuiets();

private:
	void OrderMoves(MoveList& moves);

	//Data needed for use in ordering or generating moves
	Position& position;
	int distanceFromRoot;
	const SearchData& locals;
	bool quiescence;
	MoveList& moveList;

	//Data uses for keeping track of internal values
	Stage stage;
	MoveList::iterator current;

	Move TTmove;
	Move Killer1;
	Move Killer2;

	bool skipQuiets = false;
};

Move GetHashMove(const Position& position, int depthRemaining, int distanceFromRoot);
Move GetHashMove(const Position& position, int distanceFromRoot);