#include "TTEntry.h"



TTEntry::TTEntry(uint64_t ZobristKey, int Score, int Depth, unsigned int Cutoff)
{
	key = ZobristKey;
	score = Score;
	depth = Depth;
	cutoff = Cutoff;
	ancient = false;
	bestMove = Move();
}

TTEntry::TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, unsigned int Cutoff)
{
	key = ZobristKey;
	score = Score;
	depth = Depth;
	cutoff = Cutoff;
	ancient = false;
	bestMove = best;
}


TTEntry::~TTEntry()
{
}
