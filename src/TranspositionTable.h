#pragma once
#include <vector>
#include <mutex>
#include <memory>		//required to compile with g++
#include <algorithm>
#include "TTEntry.h"

// To allocate the transposition table, we need to do so on the heap to avoid a
// stack overflow. A std::vector<T> would be a natural choice but has the one
// drawback of expensive allocation. There is no way to resize a vector while
// leaving its elements uninitialized. FastAllocateVector manages a dynamic
// array with extremely cheap reallocations when needed. The interface is
// minimal and only satisfies the requirements of a TranspositionTable class.

template <typename T>
class FastAllocateVector
{
public:
	FastAllocateVector(size_t size) { reallocate(size); }

	// make_unique<T[]>(size) will initialize the pointed to value which would defeat the purpose
	// of FastAllocateVector
	void reallocate(size_t size) { size_ = size; table.reset(new T[size]); }

	const T& operator[](size_t index) const { return table[index]; }
	      T& operator[](size_t index)       { return table[index]; }

	size_t size() const { return size_; }

private:
	std::unique_ptr<T[]> table;
	size_t size_;
};

class TranspositionTable
{
public:
	TranspositionTable() : table(FastAllocateVector<TTBucket>(CalculateSize(32))) {} //32MB default

	size_t GetSize() const { return table.size(); }
	int GetCapacity(int halfmove) const;

	void ResetTable();
	void SetSize(uint64_t MB);	//will wipe the table and reconstruct a new empty table with a set size. units in MB!
	void AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, int Turncount, int distanceFromRoot, EntryType Cutoff);
	TTEntry GetEntry(uint64_t key, int distanceFromRoot) const;
	void ResetAge(uint64_t key, int halfmove, int distanceFromRoot);
	void PreFetch(uint64_t key) const;

private:
	uint64_t HashFunction(const uint64_t& key) const;
	static constexpr uint64_t CalculateSize(uint64_t MB) { return MB * 1024 * 1024 / sizeof(TTBucket); }

	FastAllocateVector<TTBucket> table;
};

bool CheckEntry(const TTEntry& entry, uint64_t key, int depth);
bool CheckEntry(const TTEntry& entry, uint64_t key);

