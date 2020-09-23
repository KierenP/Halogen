#include "EvalCache.h"

EvalCacheTable::EvalCacheTable()
{
	size_t entries = 511 * 1024 / sizeof(EvalCacheEntry);

	for (size_t i = 0; i < entries; i++)
	{
		table.push_back({});
	}
}

EvalCacheTable::~EvalCacheTable()
{
}

void EvalCacheTable::AddEntry(uint64_t key, int eval)
{
	table[key % table.size()].key = key;
	table[key % table.size()].eval = eval;
}

bool EvalCacheTable::GetEntry(uint64_t key, int& eval)
{
	if (table[key % table.size()].key != key)
		return false;

	eval = table[key % table.size()].eval;

	return true;
}

void EvalCacheTable::Reset()
{
	for (size_t i = 0; i < table.size(); i++)
	{
		table[i] = {};
	}
}
