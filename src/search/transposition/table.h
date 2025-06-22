#pragma once
#include <cstddef>
#include <cstdint>

#include "Score.h"

class Move;
enum class SearchResultType : uint8_t;

namespace Transposition
{

class Entry;
struct Bucket;

class Table
{
public:
    Table() = default;
    ~Table();

    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;
    Table(Table&&) = delete;
    Table& operator=(Table&&) = delete;

    [[nodiscard]] int get_hashfull(int halfmove) const;

    void clear(int thread_count);

    // will wipe the table and reconstruct a new empty table with a set size. units in MB!
    void set_size(uint64_t MB, int thread_count);

    void add_entry(const Move& best, uint64_t ZobristKey, Score score, int Depth, int Turncount, int distanceFromRoot,
        SearchResultType Cutoff, Score static_eval);

    void prefetch(uint64_t key) const;

    // find a matching entry at any depth
    Entry* get_entry(uint64_t key, int distanceFromRoot, int half_turn_count);

private:
    void realloc(int thread_count);
    void dealloc();
    Bucket& get_bucket(uint64_t key) const;

    Bucket* table = nullptr;
    size_t size_ = 0;
};

}