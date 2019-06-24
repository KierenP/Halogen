#include "TranspositionTable.h"

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

/*void TranspositionTable::AddEntry(Move best, uint64_t ZobristKey, int Score, int Depth, unsigned int Cutoff)
{
	if ((table.at(ZobristKey % TTSize).GetKey() == EMPTY) || (table.at(ZobristKey % TTSize).GetDepth() <= Depth) || (table.at(ZobristKey % TTSize).IsAncient()))
		table.at(ZobristKey % TTSize) = TTEntry(best, ZobristKey, Score, Depth, Cutoff);
}*/

void TranspositionTable::AddEntry(TTEntry entry)
{
	int hash = entry.GetKey() % TTSize;

	if ((table.at(hash).GetKey() == EMPTY) || (table.at(hash).GetDepth() <= entry.GetDepth()) || (table.at(hash).IsAncient()))
		table.at(hash) = entry;
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
