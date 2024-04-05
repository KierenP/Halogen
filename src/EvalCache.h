#pragma once
#include "Score.h"
#include <cstdint>
#include <vector>

struct EvalCacheEntry
{
    uint64_t key = 0;
    Score eval = 0;
};

class EvalCacheTable
{
public:
    EvalCacheTable();

    void AddEntry(uint64_t key, Score eval);
    bool GetEntry(uint64_t key, Score& eval) const;

    void Reset();

private:
    std::vector<EvalCacheEntry> table;
};
