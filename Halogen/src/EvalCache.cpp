#include "EvalCache.h"

constexpr size_t TableSize = 65536;

EvalCacheTable::EvalCacheTable()
{
	for (size_t i = 0; i < TableSize; i++)
		table.push_back({});
}

EvalCacheTable::~EvalCacheTable()
{
}

void EvalCacheTable::AddEntry(uint64_t key, int eval)
{
	table[key % TableSize].key = key;
	table[key % TableSize].eval = eval;
}

bool EvalCacheTable::GetEntry(uint64_t key, int& eval)
{
	if (table[key % TableSize].key != key)
	{
		misses++;
		return false;
	}

	eval = table[key % TableSize].eval;
	hits++; 

	return true;
}

void EvalCacheTable::Reset()
{
	hits = 0;
	misses = 0;

	for (size_t i = 0; i < TableSize; i++)
	{
		table[i] = {};
	}
}
