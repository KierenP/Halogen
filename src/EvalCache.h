#pragma once
#include <cstdint>
#include <vector>

struct EvalCacheEntry
{
    uint64_t key = 0;
    int eval = 0;
};

class EvalCacheTable
{
public:
    EvalCacheTable();

    void AddEntry(uint64_t key, int eval);
    bool GetEntry(uint64_t key, int& eval) const;

    void Reset();

private:
    std::vector<EvalCacheEntry> table;
};
