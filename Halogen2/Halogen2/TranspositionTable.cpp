#include "TranspositionTable.h"

unsigned int TTSize = 62500;	//default is 1MB hash

TranspositionTable::TranspositionTable()
{
	TTHits = 0;

	for (int i = 0; i < TTSize; i++)
	{
		table.push_back(TTEntry());
	}
}

TranspositionTable::~TranspositionTable()
{
}

bool TranspositionTable::CheckEntry(uint64_t key, int depth)
{
	if ((table.at(key % TTSize).GetKey() == key) && (table.at(key % TTSize).GetDepth() >= depth))
		return true;
	return false;
}

bool TranspositionTable::CheckEntry(uint64_t key)
{
	if ((table.at(key % TTSize).GetKey() == key))
		return true;
	return false;
}

void TranspositionTable::AddEntry(const Move& best, uint64_t ZobristKey, int Score, int Depth, int distanceFromRoot, EntryType Cutoff)
{
	int hash = ZobristKey % TTSize;

	if (Score > 9000)	//checkmate node
		Score += distanceFromRoot;
	if (Score < -9000)
		Score -= distanceFromRoot;

	if ((table.at(hash).GetKey() == EMPTY) || (table.at(hash).GetDepth() <= Depth) || (table.at(hash).IsAncient()))
		table.at(hash) = TTEntry(best, ZobristKey, Score, Depth, Cutoff);
}

TTEntry TranspositionTable::GetEntry(uint64_t key)
{
	table.at(key % TTSize).SetAncient(false);
	return table.at(key % TTSize);
}

void TranspositionTable::SetAllAncient()
{
	for (int i = 0; i < TTSize; i++)
	{
		table.at(i).SetAncient(true);
	}
}

int TranspositionTable::GetCapacity()
{
	int count = 0;

	for (int i = 0; i < TTSize; i++)
	{
		if (!table.at(i).IsAncient())
			count++;
	}

	return count;
}

void TranspositionTable::ResetTable()
{

	TTHits = 0;

	for (int i = 0; i < TTSize; i++)
	{
		table.at(i) = TTEntry();
	}
}

void TranspositionTable::SetSize(unsigned int MB)
{
	table.clear();
	unsigned int EntrySize = sizeof(TTEntry);
	TTSize = MB * 1024 * 1024 / EntrySize;
	
	for (int i = 0; i < TTSize; i++)
	{
		table.push_back(TTEntry());
	}
}
