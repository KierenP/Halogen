#pragma once

#include "search/score.h"
#include "utility/huge_pages.h"

#include <cstddef>
#include <cstdint>
#include <memory>

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
    Bucket& get_bucket(uint64_t key) const;

    unique_ptr_huge_page<Bucket[]> table;
    size_t size_ = 0;
};

}