#include "TranspositionTable.h"

TranspositionTable::TranspositionTable()
{
	TTHits = 0;
	table.push_back(TTEntry());
}

TranspositionTable::~TranspositionTable()
{
}

bool CheckEntry(const TTEntry& entry, uint64_t key, int depth)
{
	if (entry.GetKey() == key && (entry.GetDepth() >= depth) && entry.GetCutoff() != EntryType::EMPTY_ENTRY)
		return true;
	return false;
}

uint64_t TranspositionTable::HashFunction(const uint64_t& key) const
{
	return key % table.size();
	//return key & 0x3FFFFF;
}

bool CheckEntry(const TTEntry& entry, uint64_t key)
{
	if (entry.GetKey() == key && entry.GetCutoff() != EntryType::EMPTY_ENTRY)
		return true;
	return false;
}

void TranspositionTable::AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, int halfmove, int distanceFromRoot, EntryType Cutoff)
{
	size_t hash = HashFunction(ZobristKey);

	if (Score > 9000)	//checkmate node
		Score += distanceFromRoot;
	if (Score < -9000)
		Score -= distanceFromRoot;

	std::lock_guard<std::mutex> lock(*locks.at(ZobristKey % locks.size()));

	if ((table.at(hash).GetKey() == EMPTY) || (table.at(hash).GetDepth() <= Depth) || (table.at(hash).IsAncient(halfmove, distanceFromRoot)) || table.at(hash).GetCutoff() == EntryType::EMPTY_ENTRY)
	{
		occupancy[table.at(hash).GetHalfMove()]--;
		table.at(hash) = TTEntry(best, ZobristKey, Score, Depth, halfmove, distanceFromRoot, Cutoff);
		occupancy[table.at(hash).GetHalfMove()]++;
	}
}

TTEntry TranspositionTable::GetEntry(uint64_t key)
{
	return table.at(HashFunction(key));
}

void TranspositionTable::SetNonAncient(uint64_t key, int halfmove, int distanceFromRoot)
{
	occupancy[table.at(HashFunction(key)).GetHalfMove()]--;
	table.at(HashFunction(key)).SetHalfMove(halfmove, distanceFromRoot);
	occupancy[table.at(HashFunction(key)).GetHalfMove()]++;
}

int TranspositionTable::GetCapacity(int halfmove) const
{
	return occupancy[halfmove % ((unsigned char)(-1) + 1)];
}

void TranspositionTable::ResetTable()
{
	TTHits = 0;

	for (int i = 0; i < table.size(); i++)
	{
		table.at(i).Reset();
	}

	for (int i = 0; i <= (unsigned char)(-1); i++)
	{
		occupancy[i] = 0;
	}
}

void TranspositionTable::SetSize(uint64_t MB)
{
	/*
	We can't adjust the number of entries based on the size of a mutex because this size is different under msvc (80 bytes) and g++ (8 bytes)
	*/

	table.clear();
	locks.clear();
	size_t EntrySize = sizeof(TTEntry);

	size_t entries = (MB * 1024 * 1024 / EntrySize);
	size_t mutexCount = (entries / mutex_frequency);
	
	for (size_t i = 0; i < entries; i++)
	{
		table.push_back(TTEntry());
	}

	for (size_t i = 0; i < mutexCount; i++)
	{
		locks.push_back(std::make_unique<std::mutex>());
	}
}

void TranspositionTable::PreFetch(uint64_t key) const
{
#ifdef _MSC_VER
	_mm_prefetch((char*)(&table[HashFunction(key)]), _MM_HINT_T0);
#endif
}

void TranspositionTable::RunAsserts() const
{
	for (int i = 0; i < table.size(); i++)
	{
		if (table[i].GetScore() == -30000 || table[i].GetScore() == 30000)
			std::cout << "Bad entry in table!";
	}
}
