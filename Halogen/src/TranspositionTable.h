#pragma once
#include <vector>
#include "TTEntry.h"

class TranspositionTable
{
public:
	TranspositionTable();
	~TranspositionTable();

	size_t GetSize() { return table.size(); }
	uint64_t GetHitCount() const { return TTHits; }
	int GetCapacity();

	void SetAllAncient();
	void ResetHitCount() { TTHits = 0; }
	void ResetTable();
	void SetSize(uint64_t MB);	//will wipe the table and reconstruct a new empty table with a set size. units in MB!

	bool CheckEntry(uint64_t key, int depth);
	bool CheckEntry(uint64_t key);
	void AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, int distanceFromRoot, EntryType Cutoff);
	TTEntry GetEntry(uint64_t key);	//you MUST do mate score adjustment if you are using this score in the alpha beta search! for move ordering there is no need

	void AddHit() { TTHits++; }	//this is called every time we get a position from here. We don't count it if we just used it for move ordering
	uint64_t HashFunction(const uint64_t& key) const;
	void PreFetch(uint64_t key);

	void RunAsserts();

private:
	std::vector<TTEntry> table;
	uint64_t TTHits;
};

