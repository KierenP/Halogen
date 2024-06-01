#pragma once
#include <cstddef>
#include <cstdint>
#include <memory> //required to compile with g++

#include "TTEntry.h"

class Move;

class TranspositionTable
{
public:
    ~TranspositionTable();

    size_t GetSize() const
    {
        return size_;
    }

    int GetCapacity(int halfmove) const;

    void ResetTable();

    // will wipe the table and reconstruct a new empty table with a set size. units in MB!
    void SetSize(uint64_t MB);

    void AddEntry(const Move& best, uint64_t ZobristKey, Score Score, int Depth, int Turncount, int distanceFromRoot,
        SearchResultType Cutoff);

    void PreFetch(uint64_t key) const;

    // find a matching entry at any depth
    TTEntry* GetEntry(uint64_t key, int distanceFromRoot, int half_turn_count);

private:
    uint64_t HashFunction(const uint64_t& key) const;
    void Reallocate();
    void Deallocate();

    static constexpr uint64_t CalculateEntryCount(uint64_t MB)
    {
        return MB * 1024 * 1024 / sizeof(TTBucket);
    }

    // raw array and memset allocates quicker than std::vector
    TTBucket* table;
    size_t size_;
    uint64_t hash_mask_;
};

bool CheckEntry(const TTEntry& entry, uint64_t key, int depth);
bool CheckEntry(const TTEntry& entry, uint64_t key);
