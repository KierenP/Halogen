#pragma once
#include <vector>
#include "TTEntry.h"

class TranspositionTable
{
public:
	TranspositionTable();
	~TranspositionTable();

	bool CheckEntry(uint64_t key, int depth);
	uint64_t HashFunction(const uint64_t& key);
	bool CheckEntry(uint64_t key);
	void AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, int distanceFromRoot, EntryType Cutoff);
	TTEntry GetEntry(uint64_t key);	//you MUST do mate score adjustment if you are using this score in the alpha beta search! for move ordering there is no need

	void SetAllAncient();
	int GetCapacity();
	void AddHit() { TTHits++; }	//this is called every time we get a position from here. We don't count it if we just used it for move ordering

	void ResetHitCount() { TTHits = 0; }
	uint64_t GetHitCount() const { return TTHits; } 

	void ResetTable();
	void SetSize(unsigned int MB);	//will wipe the table and reconstruct a new empty table with a set size. units in MB!
	unsigned int GetSize() { return table.size(); }

	std::vector<TTEntry> table;
private:

	uint64_t TTHits;

};

