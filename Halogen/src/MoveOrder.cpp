#include "MoveOrder.h"

enum class Stage
{
	TT_MOVE,
	LOUD_MOVES,
	KILLER_MOVES,
	BAP_CAPTURES,
	QUIET_MOVES
};


class MoveGenerator
{
public:
	MoveGenerator();
	~MoveGenerator();

	bool GetNext(Move& move, const Position& position, int distanceFromRoot);

private:
	Stage state;
};

MoveGenerator::MoveGenerator()
{
	state = Stage::TT_MOVE;
}

MoveGenerator::~MoveGenerator()
{
}

bool MoveGenerator::GetNext(Move& move, const Position& position, int distanceFromRoot)
{
	switch (state)
	{
	case Stage::TT_MOVE:
		move = GetHashMove(position, distanceFromRoot);
		if (!move.IsUninitialized())
		{	
			state = Stage::LOUD_MOVES;
			return true;
		}
		//Fall through to next case

	case Stage::LOUD_MOVES:
		break;
	case Stage::KILLER_MOVES:
		break;
	case Stage::BAP_CAPTURES:
		break;
	case Stage::QUIET_MOVES:
		break;
	default:
		break;
	}
}

Move GetHashMove(const Position& position, int distanceFromRoot)
{
	TTEntry hash = tTable.GetEntry(position.GetZobristKey());

	if (CheckEntry(hash, position.GetZobristKey()))
	{
		tTable.SetNonAncient(position.GetZobristKey(), position.GetTurnCount(), distanceFromRoot);
		return hash.GetMove();
	}

	return {};
}
