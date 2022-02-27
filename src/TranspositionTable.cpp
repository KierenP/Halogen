#include "TranspositionTable.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iterator>

#include "BitBoardDefine.h"
#include "TTEntry.h"

class Move;

bool CheckEntry(const TTEntry& entry, uint64_t key, int depth)
{
    return (entry.GetKey() == key && entry.GetDepth() >= depth);
}

uint64_t TranspositionTable::HashFunction(const uint64_t& key) const
{
    return key % size_;
}

bool CheckEntry(const TTEntry& entry, uint64_t key)
{
    return (entry.GetKey() == key);
}

void TranspositionTable::AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, int Turncount, int distanceFromRoot, EntryType Cutoff)
{
    size_t hash = HashFunction(ZobristKey);

    //checkmate node or TB win/loss
    if (Score > EVAL_MAX)
        Score += distanceFromRoot;
    if (Score < EVAL_MIN)
        Score -= distanceFromRoot;

    for (auto& entry : table[hash].entry)
    {
        if (entry.GetKey() == EMPTY || entry.GetKey() == ZobristKey)
        {
            entry = TTEntry(best, ZobristKey, Score, Depth, Turncount, distanceFromRoot, Cutoff);
            return;
        }
    }

    int8_t currentAge = TTEntry::CalculateAge(Turncount, distanceFromRoot); //Keep in mind age from each generation goes up so lower (generally) means older
    std::array<int8_t, TTBucket::SIZE> scores = {};

    for (size_t i = 0; i < TTBucket::SIZE; i++)
    {
        int8_t age_diff = currentAge - table[hash].entry[i].GetAge();
        scores[i] = table[hash].entry[i].GetDepth() - 4 * (age_diff >= 0 ? age_diff : age_diff + HALF_MOVE_MODULO);
    }

    table[hash].entry[std::distance(scores.begin(), std::min_element(scores.begin(), scores.end()))] = TTEntry(best, ZobristKey, Score, Depth, Turncount, distanceFromRoot, Cutoff);
}

TTEntry TranspositionTable::GetEntry(uint64_t key, int distanceFromRoot) const
{
    size_t index = HashFunction(key);

    for (auto entry : table[index].entry)
    {
        if (entry.GetKey() == key)
        {
            entry.MateScoreAdjustment(distanceFromRoot);
            return entry;
        }
    }

    return {};
}

void TranspositionTable::ResetAge(uint64_t key, int halfmove, int distanceFromRoot)
{
    size_t index = HashFunction(key);

    for (auto& entry : table[index].entry)
    {
        if (entry.GetKey() == key)
        {
            entry.SetHalfMove(halfmove, distanceFromRoot);
        }
    }
}

int TranspositionTable::GetCapacity(int halfmove) const
{
    int count = 0;

    for (int i = 0; i < 1000; i++) //1000 chosen specifically, because result needs to be 'per mill'
    {
        if (table[i / TTBucket::SIZE].entry[i % TTBucket::SIZE].GetAge() == static_cast<char>(halfmove % HALF_MOVE_MODULO))
            count++;
    }

    return count;
}

void TranspositionTable::ResetTable()
{
    memset(table.get(), 0, size_ * sizeof(TTBucket));
}

void TranspositionTable::SetSize(uint64_t MB)
{
    Reallocate(CalculateEntryCount(MB));
}

void TranspositionTable::Reallocate(size_t size)
{
    size_ = size;
    table.reset(new TTBucket[size]);
    ResetTable();
}

void TranspositionTable::PreFetch(uint64_t key) const
{
    __builtin_prefetch(&table[HashFunction(key)]);
}
