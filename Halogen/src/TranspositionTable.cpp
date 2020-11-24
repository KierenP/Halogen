#include "TranspositionTable.h"

TranspositionTable::TranspositionTable()
{
	table.push_back(TTBucket());
}

TranspositionTable::~TranspositionTable()
{
}

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
	if (!HASH_ENABLE)
		return;

	size_t hash = HashFunction(ZobristKey);

	if (Score > 9000)	//checkmate node
		Score += distanceFromRoot;
	if (Score < -9000)
		Score -= distanceFromRoot;

	for (int i = 0; i < 4; i++)
	{
		if (table[hash].entry[i].GetKey() == EMPTY || table[hash].entry[i].GetKey() == ZobristKey)
		{
			table[hash].entry[i] = TTEntry(best, ZobristKey, Score, Depth, Turncount, distanceFromRoot, Cutoff);
			return;
		}
	}

	int8_t currentAge = TTEntry::CalculateHalfMove(Turncount, distanceFromRoot);	//Keep in mind age from each generation goes up so lower (generally) means older
	int8_t age;

	age = currentAge - table[hash].entry[0].GetHalfMove();
	int8_t score1 = table[hash].entry[0].GetDepth() - 4 * (age >= 0 ? age : age + 16);
	age = currentAge - table[hash].entry[1].GetHalfMove();
	int8_t score2 = table[hash].entry[1].GetDepth() - 4 * (age >= 0 ? age : age + 16);
	age = currentAge - table[hash].entry[2].GetHalfMove();
	int8_t score3 = table[hash].entry[2].GetDepth() - 4 * (age >= 0 ? age : age + 16);
	age = currentAge - table[hash].entry[3].GetHalfMove();
	int8_t score4 = table[hash].entry[3].GetDepth() - 4 * (age >= 0 ? age : age + 16);

	int8_t lowest = score1;
	lowest = std::min(lowest, score2);
	lowest = std::min(lowest, score3);
	lowest = std::min(lowest, score4);

	if (lowest == score1)		table[hash].entry[0] = TTEntry(best, ZobristKey, Score, Depth, Turncount, distanceFromRoot, Cutoff);
	else if (lowest == score2)	table[hash].entry[1] = TTEntry(best, ZobristKey, Score, Depth, Turncount, distanceFromRoot, Cutoff);
	else if (lowest == score3)	table[hash].entry[2] = TTEntry(best, ZobristKey, Score, Depth, Turncount, distanceFromRoot, Cutoff);
	else						table[hash].entry[3] = TTEntry(best, ZobristKey, Score, Depth, Turncount, distanceFromRoot, Cutoff);
}

TTEntry TranspositionTable::GetEntry(uint64_t key)
{
	size_t index = HashFunction(key);

	for (int i = 0; i < 4; i++)
		if (table[index].entry[i].GetKey() == key)
			return table[index].entry[i];

	return {};
}

void TranspositionTable::SetNonAncient(uint64_t key, int halfmove, int distanceFromRoot)
{
	size_t index = HashFunction(key);

	for (int i = 0; i < 4; i++)
		if (table[index].entry[i].GetKey() == key)
			table[index].entry[i].SetHalfMove(halfmove, distanceFromRoot);
}

int TranspositionTable::GetCapacity(int halfmove) const
{
	int count = 0;

	for (int i = 0; i < 1000; i++)	//1000 chosen specifically, because result needs to be 'per mill'
	{
		if (table.at(i / 4).entry[i % 4].GetHalfMove() == static_cast<char>(halfmove % HALF_MOVE_MODULO))
			count++;
	}

	return count;
}

void TranspositionTable::ResetTable()
{
	for (size_t i = 0; i < table.size(); i++)
	{
		table.at(i).Reset();
	}
}

void TranspositionTable::SetSize(uint64_t MB)
{
	table.resize(MB * 1024 * 1024 / sizeof(TTBucket));
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
