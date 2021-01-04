#include "TTEntry.h"

TTEntry::TTEntry()
{
	key = EMPTY;
	score = -1;
	depth = -1;
	cutoff = EntryType::EMPTY_ENTRY;
	halfmove = -1;
}

TTEntry::TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, int currentTurnCount, int distanceFromRoot, EntryType Cutoff)
{
	assert(Score < SHRT_MAX && Score > SHRT_MIN);
	assert(Depth < CHAR_MAX && Depth > CHAR_MIN);

	key = ZobristKey;
	bestMove = best;
	score = static_cast<short>(Score);
	depth = static_cast<char>(Depth);
	cutoff = Cutoff;
	SetHalfMove(currentTurnCount, distanceFromRoot);
}


TTEntry::~TTEntry()
{
}

void TTEntry::MateScoreAdjustment(int distanceFromRoot)
{
	if (score > 9000)	//checkmate node
		score -= static_cast<short>(distanceFromRoot);
	if (score < -9000)
		score += static_cast<short>(distanceFromRoot);
}

void TTEntry::Reset()
{
	bestMove.Reset();
	key = EMPTY;
	score = -1;
	depth = -1;
	cutoff = EntryType::EMPTY_ENTRY;
	halfmove = -1;
}

void TTBucket::Reset()
{
	entry[0].Reset();
	entry[1].Reset();
	entry[2].Reset();
	entry[3].Reset();
}
