#pragma once
#include "Move.h"
#include "BitBoardDefine.h"

enum class EntryType {
	EMPTY_ENTRY,
	EXACT,
	LOWERBOUND,
	UPPERBOUND
};

class TTEntry
{
public:
	TTEntry();
	TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, EntryType Cutoff);
	~TTEntry();

	uint64_t GetKey() const { return key; }
	int GetScore() const { return score; } 	
	int GetDepth() const { return depth; }
	bool IsAncient() const { return ancient; }
	EntryType GetCutoff() const { return cutoff; }
	Move GetMove() const { return bestMove; }

	void SetAncient(bool isAncient) { ancient = isAncient; }
	void MateScoreAdjustment(int distanceFromRoot);

private:

	uint64_t key;
	Move bestMove;
	int score;
	int depth;
	EntryType cutoff;
	bool ancient;
};

