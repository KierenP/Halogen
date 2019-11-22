#include "TTEntry.h"

TTEntry::TTEntry() : bestMove(0, 0, 0)
{
	key = EMPTY;
	score = -1;
	depth = -1;
	cutoff = NodeCut::UNINITIALIZED_NODE;
	ancient = true;
}

TTEntry::TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, NodeCut Cutoff) : bestMove(best)
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
