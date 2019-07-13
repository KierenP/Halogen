#pragma once
#include "TTEntry.h"
#include "ABnode.h"

extern unsigned int TTSize;	//default is 1MB hash

class TranspositionTable
{
public:
	TranspositionTable();
	~TranspositionTable();

	bool CheckEntry(uint64_t key, int depth);
	void AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, unsigned int Cutoff);
	TTEntry GetEntry(uint64_t key);

	void SetAllAncient();
	int GetCapacity();
	void AddHit() { TTHits++; }

	void ResetHitCount() { TTHits = 0; }
	unsigned int GetHitCount() const { return TTHits; } 

	void ResetTable();
	void SetSize(unsigned int MB);	//will wipe the table and reconstruct a new empty table with a set size. units in MB!

private:
	unsigned int TTHits;
	std::vector<TTEntry> table;
};

