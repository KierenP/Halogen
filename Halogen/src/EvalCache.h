#pragma once
#include <vector>
#include <memory>		//required to compile with g++
#include <array>
#include <assert.h>
#include <iostream>

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

	uint64_t hits = 0;
	uint64_t misses = 0;

private:
	std::vector<EvalCacheEntry> table;
};
