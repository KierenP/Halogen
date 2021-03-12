#include "EvalCache.h"

constexpr size_t TableSize = 65536;

EvalCacheTable::EvalCacheTable()
{
	table.resize(TableSize);
}

void EvalCacheTable::AddEntry(uint64_t key, int eval)
{
	table[key % TableSize].key = key;
	table[key % TableSize].eval = eval;
}

bool EvalCacheTable::GetEntry(uint64_t key, int& eval) const
{
	if (table[key % TableSize].key != key)
	{
		return false;
	}

	eval = table[key % TableSize].eval;
	return true;
}

void EvalCacheTable::Reset()
{
	table.clear();
	table.resize(TableSize);
}
