#pragma once
#include "TTEntry.h"
#include "ABnode.h"

const unsigned int TTSize = 67108864;						//2^26 * 16 bytes = 2^30 = 1GB

class TranspositionTable
{
public:
	TranspositionTable();
	~TranspositionTable();

	bool CheckEntry(uint64_t key, int depth);
	void AddEntry(Move best, uint64_t ZobristKey, int Score, int Depth, unsigned int Cutoff);
	TTEntry GetEntry(uint64_t key);

	void Reformat();
	void AddHit() { TTHits++; }

	void ResetCount() { TTHits = 0; }
	unsigned int GetCount() const { return TTHits; } 

private:
	unsigned int TTHits;
	TTEntry table[TTSize];
};

