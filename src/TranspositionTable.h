#pragma once
#include <cstddef>
#include <cstdint>
#include <memory> //required to compile with g++

#include "TTEntry.h"

class Move;

class TranspositionTable
{
public:
    TranspositionTable()
        : size_(CalculateSize(32))
        , table(new TTBucket[size_])
    {
    } //32MB default

    size_t GetSize() const { return size_; }
    int GetCapacity(int halfmove) const;

    void ResetTable();
    void SetSize(uint64_t MB); //will wipe the table and reconstruct a new empty table with a set size. units in MB!
    void AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, int Turncount, int distanceFromRoot, EntryType Cutoff);
    TTEntry GetEntry(uint64_t key, int distanceFromRoot) const;
    void ResetAge(uint64_t key, int halfmove, int distanceFromRoot);
    void PreFetch(uint64_t key) const;

private:
    uint64_t HashFunction(const uint64_t& key) const;
    void Reallocate(size_t size);

    static constexpr uint64_t CalculateSize(uint64_t MB) { return MB * 1024 * 1024 / sizeof(TTBucket); }

    size_t size_;
    std::unique_ptr<TTBucket[]> table;
};

bool CheckEntry(const TTEntry& entry, uint64_t key, int depth);
bool CheckEntry(const TTEntry& entry, uint64_t key);
