#pragma once
#include "Move.h"
#include "BitBoardDefine.h"

enum EntryType {
	EMPTY_ENTRY,
	EXACT,
	LOWERBOUND,
	UPPERBOUND
};

class TTEntry
{
public:
	TTEntry();
	TTEntry(Move best, uint64_t ZobristKey, short int Score, short int Depth, char Cutoff);
	~TTEntry();

	uint64_t GetKey() const { return key; }
	int GetScore() const { return score; } 	
	int GetDepth() const { return depth; }
	bool IsAncient() const { return ancient; }
	char GetCutoff() const { return cutoff; }
	Move GetMove() const { return bestMove; }

	void SetAncient(bool isAncient) { ancient = isAncient; }
	void MateScoreAdjustment(int distanceFromRoot);
	void DrawScoreAdjustment() { score /= 2; }

private:

	uint64_t key;
	Move bestMove;
	int score;
	int depth;
	char cutoff;
	bool ancient;

};

