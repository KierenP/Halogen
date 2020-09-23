#pragma once
#include <vector>
#include <memory>		//required to compile with g++

struct EvalCacheEntry
{
	uint64_t key = 0;
	int eval = -1;
};

class EvalCacheTable
{
public:
	EvalCacheTable();
	~EvalCacheTable();

	void AddEntry(uint64_t key, int eval);
	bool GetEntry(uint64_t key, int& eval);

	void Reset();

private:
	std::vector<EvalCacheEntry> table;
};
