#pragma once
#include "TTEntry.h"

const unsigned int TTSize = 67108864;			//2^26 * 16 bytes = 2^30 = 1GB

class TranspositionTable
{
public:
	TranspositionTable();
	~TranspositionTable();

	bool CheckEntry(uint64_t key, int depth);
	//void AddEntry(ABnode best, uint64_t ZobristKey);
	void AddEntry(Move best, uint64_t ZobristKey, int Score, int Depth, unsigned int Cutoff);
	TTEntry GetEntry(uint64_t key);

	void Reformat();

	unsigned int TTHits;

private:
	TTEntry table[TTSize];
};

