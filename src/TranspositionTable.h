#pragma once
#include <cstddef>
#include <cstdint>

#include "Score.h"

class Move;
class TTEntry;
enum class SearchResultType : uint8_t;
struct TTBucket;

class TranspositionTable
{
public:
    TranspositionTable() = default;
    ~TranspositionTable();

    TranspositionTable(const TranspositionTable&) = delete;
    TranspositionTable& operator=(const TranspositionTable&) = delete;
    TranspositionTable(TranspositionTable&&) = delete;
    TranspositionTable& operator=(TranspositionTable&&) = delete;

    size_t GetSize() const
    {
        return size_;
    }

    int GetCapacity(int halfmove) const;

    void Clear(int thread_count);

    // will wipe the table and reconstruct a new empty table with a set size. units in MB!
    void SetSize(uint64_t MB, int thread_count);

    void AddEntry(const Move& best, uint64_t ZobristKey, Score score, int Depth, int Turncount, int distanceFromRoot,
        SearchResultType Cutoff, Score static_eval);

    void PreFetch(uint64_t key) const;

    // find a matching entry at any depth
    TTEntry* GetEntry(uint64_t key, int distanceFromRoot, int half_turn_count);

private:
    void Reallocate(int thread_count);
    void Deallocate();
    TTBucket& GetBucket(uint64_t key) const;

    // raw array and memset allocates quicker than std::vector
    TTBucket* table = nullptr;
    size_t size_ = 0;
};
