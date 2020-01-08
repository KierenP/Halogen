#include "TTEntry.h"

TTEntry::TTEntry() : bestMove(0, 0, 0)
{
	key = EMPTY;
	score = -1;
	depth = -1;
	cutoff = EntryType::EMPTY_ENTRY;
	ancient = true;
}

TTEntry::TTEntry(Move best, uint64_t ZobristKey, short int Score, short int Depth, char Cutoff) : bestMove(best)
{
	key = ZobristKey;
	score = Score;
	depth = Depth;
	cutoff = Cutoff;
	ancient = false;
}


TTEntry::~TTEntry()
{
}

void TTEntry::MateScoreAdjustment(int distanceFromRoot)
{
	if (GetScore() > 9000)	//checkmate node
		score -= distanceFromRoot;
	if (GetScore() < -9000)
		score += distanceFromRoot;
}
