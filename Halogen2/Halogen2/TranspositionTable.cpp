#include "TranspositionTable.h"

TranspositionTable::TranspositionTable()
{
	TTHits = 0;

	for (int i = 0; i < TTSize; i++)
	{
		table[i] = TTEntry();
	}
}

TranspositionTable::~TranspositionTable()
{
}

bool TranspositionTable::CheckEntry(uint64_t key, int depth)
{
	if ((table[key % TTSize].GetKey() == key) && (table[key % TTSize].GetDepth() >= depth))
		return true;
	return false;
}

/*void TranspositionTable::AddEntry(Move best, uint64_t ZobristKey, int Score, int Depth, unsigned int Cutoff)
{
	if ((table[ZobristKey % TTSize].GetKey() == EMPTY) || (table[ZobristKey % TTSize].GetDepth() <= Depth) || (table[ZobristKey % TTSize].IsAncient()))
		table[ZobristKey % TTSize] = TTEntry(best, ZobristKey, Score, Depth, Cutoff);
}*/

void TranspositionTable::AddEntry(TTEntry entry)
{
	int hash = entry.GetKey() % TTSize;

	if ((table[hash].GetKey() == EMPTY) || (table[hash].GetDepth() <= entry.GetDepth()) || (table[hash].IsAncient()))
		table[hash] = entry;
}

TTEntry TranspositionTable::GetEntry(uint64_t key)
{
	table[key % TTSize].SetAncient(false);
	return table[key % TTSize];
}

void TranspositionTable::SetAllAncient()
{
	for (int i = 0; i < TTSize; i++)
	{
		table[i].SetAncient(true);
	}
}

int TranspositionTable::GetCapacity()
{
	int count = 0;

	for (int i = 0; i < TTSize; i++)
	{
		if (!table[i].IsAncient())
			count++;
	}

	return count;
}

void TranspositionTable::ResetTable()
{
	TTHits = 0;

	for (int i = 0; i < TTSize; i++)
	{
		table[i] = TTEntry();
	}
}
