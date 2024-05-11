#include "TranspositionTable.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <memory>

#ifdef __linux__
#include <sys/mman.h>
#endif

#include "BitBoardDefine.h"
#include "TTEntry.h"

TranspositionTable ::~TranspositionTable()
{
    Deallocate();
}

uint64_t TranspositionTable::HashFunction(const uint64_t& key) const
{
    return key & hash_mask_;
}

void TranspositionTable::AddEntry(const Move& best, uint64_t ZobristKey, Score score, int Depth, int Turncount,
    int distanceFromRoot, EntryType Cutoff)
{
    size_t hash = HashFunction(ZobristKey);
    score = convert_to_tt_score(score, distanceFromRoot);

    // Keep in mind age from each generation goes up so lower (generally) means older
    int8_t current_generation = get_generation(Turncount, distanceFromRoot);
    std::array<int8_t, TTBucket::size> scores = {};
    auto& bucket = table[hash];

    const auto write_to_entry = [&](auto& entry)
    {
        entry.move = best;
        entry.key = ZobristKey;
        entry.score = score;
        entry.depth = Depth;
        entry.cutoff = Cutoff;
        entry.generation = current_generation;
    };

    for (size_t i = 0; i < TTBucket::size; i++)
    {
        // each bucket fills from the first entry, and only once all entries are full do we use the replacement scheme
        if (bucket[i].key == EMPTY)
        {
            write_to_entry(bucket[i]);
            return;
        }

        // avoid having multiple entries in a bucket for the same position.
        if (bucket[i].key == ZobristKey)
        {
            // always replace if exact, or if the depth is sufficiently high. There's a trade-off here between wanting
            // to save the higher depth entry, and wanting to save the newer entry (which might have better bounds)
            if (Cutoff == EntryType::EXACT || Depth >= bucket[i].depth - 3)
            {
                write_to_entry(bucket[i]);
            }
            return;
        }

        int8_t age_diff = current_generation - bucket[i].generation;
        scores[i] = bucket[i].depth - 4 * (age_diff >= 0 ? age_diff : age_diff + HALF_MOVE_MODULO);
    }

    write_to_entry(bucket[std::distance(scores.begin(), std::min_element(scores.begin(), scores.end()))]);
}

TTEntry* TranspositionTable::GetEntry(uint64_t key, int distanceFromRoot, int half_turn_count)
{
    size_t index = HashFunction(key);

    // we return by copy here because other threads are reading/writing to this same table.
    for (auto& entry : table[index])
    {
        if (entry.key == key)
        {
            // reset the age of this entry to mark it as not old
            entry.generation = get_generation(half_turn_count, distanceFromRoot);
            return &entry;
        }
    }

    return nullptr;
}

int TranspositionTable::GetCapacity(int halfmove) const
{
    int count = 0;

    for (int i = 0; i < 1000; i++) // 1000 chosen specifically, because result needs to be 'per mill'
    {
        if (table[i / TTBucket::size][i % TTBucket::size].generation == get_generation(halfmove, 0))
            count++;
    }

    return count;
}

void TranspositionTable::ResetTable()
{
    Reallocate();
}

void TranspositionTable::SetSize(uint64_t MB)
{
    size_ = CalculateEntryCount(MB);
    hash_mask_ = size_ - 1;
    Reallocate();

    // size should be a power of two
    assert(GetBitCount(size_) == 1);
}

void TranspositionTable::Reallocate()
{
    Deallocate();

#ifdef __linux__
    constexpr static size_t huge_page_size = 2 * 1024 * 1024;
    const size_t bytes = size_ * sizeof(TTBucket);
    table = static_cast<TTBucket*>(std::aligned_alloc(huge_page_size, bytes));
    madvise(table, bytes, MADV_HUGEPAGE);
    std::uninitialized_default_construct_n(table, size_);
#else
    table = new TTBucket[size_];
#endif
}

void TranspositionTable::Deallocate()
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

void TranspositionTable::PreFetch(uint64_t key) const
{
    __builtin_prefetch(&table[HashFunction(key)]);
}
