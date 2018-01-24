#include "TTEntry.h"

TTEntry::TTEntry() : bestMove(0, 0, 0)
{
	key = EMPTY;
	score = -1;
	depth = -1;
	cutoff = -1;
	ancient = true;
}

TTEntry::TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, unsigned int Cutoff) : bestMove(best)
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
