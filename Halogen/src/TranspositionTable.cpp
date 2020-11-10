#include "TranspositionTable.h"

TranspositionTable::TranspositionTable()
{
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

void TranspositionTable::AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, int Turncount, int distanceFromRoot, EntryType Cutoff)
{
	if (!HASH_ENABLE)
		return;

	size_t hash = HashFunction(ZobristKey);

	if (Score > 9000)	//checkmate node
		Score += distanceFromRoot;
	if (Score < -9000)
		Score -= distanceFromRoot;


	if ((table.at(hash).GetKey() == EMPTY) || (table.at(hash).GetDepth() <= Depth) || (table.at(hash).IsAncient(Turncount, distanceFromRoot)) || table.at(hash).GetCutoff() == EntryType::EMPTY_ENTRY)
	{
		table.at(hash) = TTEntry(best, ZobristKey, Score, Depth, Turncount, distanceFromRoot, Cutoff);
	}
}

TTEntry TranspositionTable::GetEntry(uint64_t key)
{
	return table.at(HashFunction(key));
}

void TranspositionTable::SetNonAncient(uint64_t key, int halfmove, int distanceFromRoot)
{
	table.at(HashFunction(key)).SetHalfMove(halfmove, distanceFromRoot);
}

int TranspositionTable::GetCapacity(int halfmove) const
{
	int count = 0;

	for (int i = 0; i < 1000; i++)	//1000 chosen specifically, because result needs to be 'per mill'
	{
		if (table.at(i).GetHalfMove() == static_cast<char>(halfmove % HALF_MOVE_MODULO))
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
	/*
	We can't adjust the number of entries based on the size of a mutex because this size is different under msvc (80 bytes) and g++ (8 bytes)
	*/

	table.clear();
	size_t EntrySize = sizeof(TTEntry);

	size_t entries = (MB * 1024 * 1024 / EntrySize);
	
	for (size_t i = 0; i < entries; i++)
	{
		table.push_back(TTEntry());
	}
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
