#include "EvalCache.h"

EvalCacheTable::EvalCacheTable()
{
	table = new std::array<EvalCacheEntry, 65536>;
}

EvalCacheTable::~EvalCacheTable()
{
	delete table;
}

void EvalCacheTable::AddEntry(uint64_t key, int eval)
{
	(*table)[key % table->size()].key = key;
	(*table)[key % table->size()].eval = eval;
}

bool EvalCacheTable::GetEntry(uint64_t key, int& eval)
{
	if ((*table)[key % table->size()].key != key)
	{
		misses++;
		return false;
	}

	eval = (*table)[key % table->size()].eval;
	hits++; 

	return true;
}

void EvalCacheTable::Reset()
{
	hits = 0;
	misses = 0;

	for (size_t i = 0; i < table->size(); i++)
	{
		(*table)[i] = {};
	}
}
