#include "search/transposition/table.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <thread>
#include <vector>

#ifdef __linux__
#include <sys/mman.h>
#endif

#include "bitboard/define.h"
#include "movegen/move.h"
#include "search/transposition/entry.h"

namespace Transposition
{

Table::~Table()
{
    dealloc();
}

void Table::add_entry(const Move& best, uint64_t ZobristKey, Score score, int Depth, int Turncount,
    int distanceFromRoot, SearchResultType Cutoff, Score static_eval)
{
    score = convert_to_tt_score(score, distanceFromRoot);
    auto key16 = uint16_t(ZobristKey);
    auto current_generation = get_generation(Turncount, distanceFromRoot);
    std::array<int16_t, Bucket::size> scores = {};
    auto& bucket = get_bucket(ZobristKey);

    const auto write_to_entry = [&](auto& entry)
    {
        // in q-search we want to avoid overwriting the best move if we don't have one
        if (best != Move::Uninitialized || entry.key != key16)
        {
            entry.move = best;
        }

        entry.key = key16;
        entry.score = score;
        entry.static_eval = static_eval;
        entry.depth = Depth;
        entry.meta = Meta { Cutoff, (uint8_t)current_generation };
    };

    for (size_t i = 0; i < Bucket::size; i++)
    {
        // each bucket fills from the first entry, and only once all entries are full do we use the replacement scheme
        if (bucket[i].key == EMPTY)
        {
            write_to_entry(bucket[i]);
            return;
        }

        // avoid having multiple entries in a bucket for the same position.
        if (bucket[i].key == key16)
        {
            // always replace if exact, or if the depth is sufficiently high. There's a trade-off here between wanting
            // to save the higher depth entry, and wanting to save the newer entry (which might have better bounds)
            if (Cutoff == SearchResultType::EXACT || Depth >= bucket[i].depth - 3)
            {
                write_to_entry(bucket[i]);
            }
            return;
        }

        Meta meta = bucket[i].meta;
        int8_t age_diff = (int8_t)current_generation - (int8_t)meta.generation;
        scores[i] = bucket[i].depth - 4 * (age_diff >= 0 ? age_diff : age_diff + GENERATION_MAX);
    }

    write_to_entry(bucket[std::distance(scores.begin(), std::min_element(scores.begin(), scores.end()))]);
}

Entry* Table::get_entry(uint64_t key, int distanceFromRoot, int half_turn_count)
{
    auto& bucket = get_bucket(key);
    auto key16 = uint16_t(key);

    // we return by copy here because other threads are reading/writing to this same table.
    for (auto& entry : bucket)
    {
        if (entry.key == key16)
        {
            // reset the age of this entry to mark it as not old
            entry.meta.generation = get_generation(half_turn_count, distanceFromRoot);
            return &entry;
        }
    }

    return nullptr;
}

int Table::get_hashfull(int halfmove) const
{
    int count = 0;
    int8_t current_generation = get_generation(halfmove, 0);

    // 1000 chosen specifically, because result needs to be 'per mill'
    for (int i = 0; i < 1000; i++)
    {
        auto& entry = table[i / Bucket::size][i % Bucket::size];
        if (entry.key != EMPTY && entry.meta.generation == current_generation)
        {
            count++;
        }
    }

    return count;
}

void Table::clear(int thread_count)
{
    // For extremely large hash sizes, we clear the table using multiple threads

    std::vector<std::thread> threads;

    for (int i = 0; i < thread_count; i++)
    {
        threads.emplace_back(
            [this, i, thread_count]()
            {
                const size_t begin = (size_ / thread_count) * i;
                const size_t end = i + 1 != thread_count ? size_ / thread_count * (i + 1) : size_;
                std::fill(&table[begin], &table[end], Bucket {});
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }
}

void Table::set_size(uint64_t MB, int thread_count)
{
    size_ = MB * 1024 * 1024 / sizeof(Bucket);
    realloc(thread_count);
}

void Table::realloc(int thread_count)
{
    dealloc();

#ifdef __linux__
    constexpr static size_t huge_page_size = 2 * 1024 * 1024;
    const size_t bytes = size_ * sizeof(Bucket);
    table = static_cast<Bucket*>(std::aligned_alloc(huge_page_size, bytes));
    madvise(table, bytes, MADV_HUGEPAGE);
    std::uninitialized_default_construct_n(table, size_);
#else
    table = new Bucket[size_];
#endif

    clear(thread_count);
}

void Table::dealloc()
{
#ifdef __linux__
    if (table)
    {
        std::destroy_n(table, size_);
        std::free(table);
    }
#else
    if (table)
    {
        delete[] table;
    }
#endif
}

void Table::prefetch(uint64_t key) const
{
    __builtin_prefetch(&get_bucket(key));
}

size_t tt_index(uint64_t key, size_t tt_size)
{
    // multiply the key by tt_size and extract out the highest order 64 bits. This gives a uniform distribution where
    // the index is determined by the higher order bits of key, for any tt_size

#if defined(__GNUC__) && defined(__x86_64__)
    __extension__ using uint128 = unsigned __int128;
    return (uint128(key) * uint128(tt_size)) >> 64;
#else
    uint64_t aL = uint32_t(key), aH = key >> 32;
    uint64_t bL = uint32_t(tt_size), bH = tt_size >> 32;
    uint64_t c1 = (aL * bL) >> 32;
    uint64_t c2 = aH * bL + c1;
    uint64_t c3 = aL * bH + uint32_t(c2);
    return aH * bH + (c2 >> 32) + (c3 >> 32);
#endif
}

Bucket& Table::get_bucket(uint64_t key) const
{
    return table[tt_index(key, size_)];
}

}