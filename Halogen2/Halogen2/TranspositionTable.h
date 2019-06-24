#pragma once
#include "TTEntry.h"
#include "ABnode.h"

const unsigned int TTSize = 8388608;						//2^23 * 16 bytes = 2^23 * 2^4 = 2^27 = 1/8 GB or 128MB

class TranspositionTable
{
public:
	TranspositionTable();
	~TranspositionTable();

	bool CheckEntry(uint64_t key, int depth);
	//void AddEntry(Move best, uint64_t ZobristKey, int Score, int Depth, unsigned int Cutoff);
	void AddEntry(TTEntry entry);
	TTEntry GetEntry(uint64_t key);

	void SetAllAncient();
	int GetCapacity();
	void AddHit() { TTHits++; }

	void ResetHitCount() { TTHits = 0; }
	unsigned int GetHitCount() const { return TTHits; } 

	void ResetTable();

private:
	unsigned int TTHits;
	std::vector<TTEntry> table;
};

