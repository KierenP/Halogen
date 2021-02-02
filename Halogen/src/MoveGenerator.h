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

struct ExtendedMove
{
	ExtendedMove(const Move _move, const int _score = 0, const short int _SEE = 0) : move(_move), score(_score), SEE(_SEE) {}

	bool operator<(const ExtendedMove& rhs) const { return score < rhs.score; };
	bool operator>(const ExtendedMove& rhs) const { return score > rhs.score; };

	Move move;
	int16_t score;
	int16_t SEE;
};

class MoveGenerator
{
public:
	MoveGenerator(Position& Position, int DistanceFromRoot, const SearchData& Locals, bool Quiescence);
	bool Next(Move& move);	//returns false if no more legal moves
	int GetSEE() const { return (current - 1)->SEE; }

	void AdjustHistory(const Move& move, SearchData& Locals, int depthRemaining) const;

private:
	void OrderMoves(std::vector<ExtendedMove>& moves);
	
	void CreateExtendedMoveVector(const std::vector<Move>& moves);

	//Data needed for use in ordering or generating moves
	Position& position;
	int distanceFromRoot;
	const SearchData& locals;
	bool quiescence;

	//Data uses for keeping track of internal values
	Stage stage;
	std::vector<ExtendedMove> legalMoves;
	std::vector<ExtendedMove>::iterator current;

	Move TTmove;
	Move Killer1;
	Move Killer2;
};

Move GetHashMove(const Position& position, int depthRemaining, int distanceFromRoot);
Move GetHashMove(const Position& position, int distanceFromRoot);

int see(Position& position, Square square, Players side);
int seeCapture(Position& position, const Move& move); //Don't send this an en passant move!

