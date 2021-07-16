#include "TranspositionTable.h"

bool CheckEntry(const TTEntry& entry, uint64_t key, int depth)
{
	return (entry.GetKey() == key && entry.GetDepth() >= depth);
}

uint64_t TranspositionTable::HashFunction(const uint64_t& key) const
{
	return key % table.size();
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

	int8_t currentAge = TTEntry::CalculateAge(Turncount, distanceFromRoot);	//Keep in mind age from each generation goes up so lower (generally) means older
	std::array<int8_t, BucketSize> scores = {};

	for (size_t i = 0; i < BucketSize; i++)
	{
		int8_t age = currentAge - table[hash].entry[i].GetAge();
		scores[i] = table[hash].entry[i].GetDepth() - 4 * (age >= 0 ? age : age + 16);
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

	for (int i = 0; i < 1000; i++)	//1000 chosen specifically, because result needs to be 'per mill'
	{
		if (table[i / BucketSize].entry[i % BucketSize].GetAge() == static_cast<char>(halfmove % HALF_MOVE_MODULO))
			count++;
	}

	return count;
}

void TranspositionTable::ResetTable()
{
	table.reallocate(table.size());
}

void TranspositionTable::SetSize(uint64_t MB)
{
	table.reallocate(CalculateSize(MB));
}

void TranspositionTable::PreFetch(uint64_t key) const
{
#ifdef _MSC_VER
	_mm_prefetch((char*)(&table[HashFunction(key)]), _MM_HINT_T0);
#endif

#ifndef _MSC_VER
	__builtin_prefetch(&table[HashFunction(key)]);
#endif 
}
