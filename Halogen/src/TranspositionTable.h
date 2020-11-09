#pragma once
#include <vector>
#include <mutex>
#include <memory>		//required to compile with g++
#include "TTEntry.h"

const unsigned int mutex_frequency = 1024;					//how many entries per mutex

class TranspositionTable
{
public:
	TranspositionTable();
	~TranspositionTable();

	size_t GetSize() const { return table.size(); }
	int GetCapacity(int halfmove) const;

	void ResetTable();
	void SetSize(uint64_t MB);	//will wipe the table and reconstruct a new empty table with a set size. units in MB!
	void AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, int Turncount, int distanceFromRoot, EntryType Cutoff);
	TTEntry GetEntry(uint64_t key);	//you MUST do mate score adjustment if you are using this score in the alpha beta search! for move ordering there is no need

	void SetNonAncient(uint64_t key, int halfmove, int distanceFromRoot);

	uint64_t HashFunction(const uint64_t& key) const;
	void PreFetch(uint64_t key) const;

private:
	std::vector<TTEntry> table;
};

bool CheckEntry(const TTEntry& entry, uint64_t key, int depth);
bool CheckEntry(const TTEntry& entry, uint64_t key);

