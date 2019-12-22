#pragma once
#include "TTEntry.h"

extern unsigned int TTSize;	//default is 1MB hash

class TranspositionTable
{
public:
	TranspositionTable();
	~TranspositionTable();

	bool CheckEntry(uint64_t key, int depth);
	void AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, int distanceFromRoot, EntryType Cutoff);
	TTEntry GetEntry(uint64_t key);	//you MUST do mate score adjustment if you are using this score in the alpha beta search! for move ordering there is no need

	void SetAllAncient();
	int GetCapacity();
	void AddHit() { TTHits++; }	//this is called every time we get a position from here. We don't count it if we just used it for move ordering

	void ResetHitCount() { TTHits = 0; }
	unsigned int GetHitCount() const { return TTHits; } 

	void ResetTable();
	void SetSize(unsigned int MB);	//will wipe the table and reconstruct a new empty table with a set size. units in MB!

private:
	uint64_t TTHits;
	std::vector<TTEntry> table;
};

