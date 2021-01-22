#pragma once
#include <vector>
#include <mutex>
#include <memory>		//required to compile with g++
#include <algorithm>
#include "TTEntry.h"

class TranspositionTable
{
public:
	TranspositionTable() { SetSize(32);	};
	~TranspositionTable() = default;

	size_t GetSize() const { return table.size(); }
	int GetCapacity(int halfmove) const;

	void ResetTable();
	void SetSize(uint64_t MB);	//will wipe the table and reconstruct a new empty table with a set size. units in MB!
	void AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, int Turncount, int distanceFromRoot, EntryType Cutoff);
	TTEntry GetEntry(uint64_t key, int distanceFromRoot);
	void ResetAge(uint64_t key, int halfmove, int distanceFromRoot);
	void PreFetch(uint64_t key) const;

private:
	uint64_t HashFunction(const uint64_t& key) const;

	std::vector<TTBucket> table;
};

bool CheckEntry(const TTEntry& entry, uint64_t key, int depth);
bool CheckEntry(const TTEntry& entry, uint64_t key);

