#pragma once
#include <vector>
#include <mutex>
#include <deque>
#include <memory>
#include "TTEntry.h"

const unsigned int mutex_frequency = 1024;					//how many entries per mutex

class TranspositionTable
{
public:
	TranspositionTable();
	~TranspositionTable();

	size_t GetSize() const { return table.size(); }
	uint64_t GetHitCount() const { return TTHits; }
	int GetCapacity() const;

	void SetAllAncient();
	void ResetHitCount() { TTHits = 0; }
	void ResetTable();
	void SetSize(uint64_t MB);	//will wipe the table and reconstruct a new empty table with a set size. units in MB!
	void AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, int distanceFromRoot, EntryType Cutoff);
	TTEntry GetEntry(uint64_t key);	//you MUST do mate score adjustment if you are using this score in the alpha beta search! for move ordering there is no need

	void AddHit() { TTHits++; }	//this is called every time we get a position from here. We don't count it if we just used it for move ordering
	uint64_t HashFunction(const uint64_t& key) const;
	void PreFetch(uint64_t key) const;

	void RunAsserts() const;

private:
	std::vector<TTEntry> table;
	std::vector<std::unique_ptr<std::mutex>> locks;
	uint64_t TTHits;
};

bool CheckEntry(const TTEntry& entry, uint64_t key, int depth);
bool CheckEntry(const TTEntry& entry, uint64_t key);

