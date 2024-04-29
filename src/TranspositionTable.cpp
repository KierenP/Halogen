#include "TranspositionTable.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iterator>
#include <optional>

#include "BitBoardDefine.h"
#include "TTEntry.h"

uint64_t TranspositionTable::HashFunction(const uint64_t& key) const
{
    return key & hash_mask_;
}

void TranspositionTable::AddEntry(const Move& best, uint64_t ZobristKey, Score score, int Depth, int Turncount,
    int distanceFromRoot, EntryType Cutoff)
{
    size_t hash = HashFunction(ZobristKey);

    // checkmate node or TB win/loss
    // TODO: check for boundary conditions
    if (score > Score::Limits::EVAL_MAX)
        score += distanceFromRoot;
    if (score < Score::Limits::EVAL_MIN)
        score -= distanceFromRoot;

    // Keep in mind age from each generation goes up so lower (generally) means older
    int8_t currentAge = TTEntry::CalculateAge(Turncount, distanceFromRoot);
    std::array<int8_t, TTBucket::SIZE> scores = {};
    auto& bucket = table[hash].entry;

    for (size_t i = 0; i < TTBucket::SIZE; i++)
    {
        // each bucket fills from the first entry, and only once all entries are full do we use the replacement scheme
        if (bucket[i].GetKey() == EMPTY)
        {
            bucket[i] = TTEntry(best, ZobristKey, score, Depth, Turncount, distanceFromRoot, Cutoff);
            return;
        }

        // avoid having multiple entries in a bucket for the same position.
        if (bucket[i].GetKey() == ZobristKey)
        {
            // always replace if exact, or if the depth is sufficiently high. There's a trade-off here between wanting
            // to save the higher depth entry, and wanting to save the newer entry (which might have better bounds)
            if (Cutoff == EntryType::EXACT || Depth >= bucket[i].GetDepth() - 3)
            {
                bucket[i] = TTEntry(best, ZobristKey, score, Depth, Turncount, distanceFromRoot, Cutoff);
            }
            return;
        }

        int8_t age_diff = currentAge - bucket[i].GetAge();
        scores[i] = bucket[i].GetDepth() - 4 * (age_diff >= 0 ? age_diff : age_diff + HALF_MOVE_MODULO);
    }

    bucket[std::distance(scores.begin(), std::min_element(scores.begin(), scores.end()))]
        = TTEntry(best, ZobristKey, score, Depth, Turncount, distanceFromRoot, Cutoff);
}

std::optional<TTEntry> TranspositionTable::GetEntry(uint64_t key, int distanceFromRoot, int half_turn_count)
{
    size_t index = HashFunction(key);

    // we return by copy here because other threads are reading/writing to this same table.
    for (auto& entry : table[index].entry)
    {
        if (entry.GetKey() == key)
        {
            // reset the age of this entry to mark it as not old
            entry.SetHalfMove(half_turn_count, distanceFromRoot);
            auto copy = entry;
            copy.MateScoreAdjustment(distanceFromRoot);
            return copy;
        }
    }

    return std::nullopt;
}

std::optional<TTEntry> TranspositionTable::GetEntryMinDepth(
    uint64_t key, int min_depth, int distanceFromRoot, int half_turn_count)
{
    size_t index = HashFunction(key);

    // we return by copy here because other threads are reading/writing to this same table.
    for (auto& entry : table[index].entry)
    {
        if (entry.GetKey() == key && entry.GetDepth() >= min_depth)
        {
            // reset the age of this entry to mark it as not old
            entry.SetHalfMove(half_turn_count, distanceFromRoot);
            auto copy = entry;
            copy.MateScoreAdjustment(distanceFromRoot);
            return copy;
        }
    }

    return std::nullopt;
}

int TranspositionTable::GetCapacity(int halfmove) const
{
    int count = 0;

    for (int i = 0; i < 1000; i++) // 1000 chosen specifically, because result needs to be 'per mill'
    {
        if (table[i / TTBucket::SIZE].entry[i % TTBucket::SIZE].GetAge() == TTEntry::CalculateAge(halfmove, 0))
            count++;
    }

    return count;
}

void TranspositionTable::ResetTable()
{
    std::fill(table.get(), table.get() + size_, TTBucket {});
}

void TranspositionTable::SetSize(uint64_t MB)
{
    Reallocate(CalculateEntryCount(MB));
}

void TranspositionTable::Reallocate(size_t size)
{
    // size should be a power of two
    assert(GetBitCount(size) == 1);
    size_ = size;
    hash_mask_ = size - 1;
    table.reset(new TTBucket[size]);
}

void TranspositionTable::PreFetch(uint64_t key) const
{
    __builtin_prefetch(&table[HashFunction(key)]);
}
