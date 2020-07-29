#include "TTEntry.h"

TTEntry::TTEntry() : bestMove(0, 0, 0)
{
	key = EMPTY;
	score = -1;
	depth = -1;
	cutoff = static_cast<char>(EntryType::EMPTY_ENTRY);
	halfmove = -1;
}

TTEntry::TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, int currentHalfMove, int distanceFromRoot, EntryType Cutoff) : bestMove(best)
{
	key = ZobristKey;
	score = Score;
	depth = Depth;
	cutoff = static_cast<char>(Cutoff);
	SetHalfMove(currentHalfMove, distanceFromRoot);
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

void TTEntry::Reset()
{
	bestMove.Reset();
	key = EMPTY;
	score = -1;
	depth = -1;
	cutoff = static_cast<char>(EntryType::EMPTY_ENTRY);
	halfmove = -1;
}
