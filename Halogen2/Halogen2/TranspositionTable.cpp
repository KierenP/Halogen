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
	if (table[key % TTSize].GetKey() == key && table[key % TTSize].GetDepth() >= depth)
		return true;
	return false;
}

/*void TranspositionTable::AddEntry(ABnode best, uint64_t ZobristKey)
{
	if ((table[ZobristKey % TTSize].GetKey() == EMPTY) || (table[ZobristKey % TTSize].GetDepth() < best.GetDepth()) || (table[ZobristKey % TTSize].IsAncient()))
		table[ZobristKey % TTSize] = TTEntry(best.GetMove(), ZobristKey, best.GetScore(), best.GetDepth(), best.GetCutoff());
}*/

void TranspositionTable::AddEntry(Move best, uint64_t ZobristKey, int Score, int Depth, unsigned int Cutoff)
{
	if ((table[ZobristKey % TTSize].GetKey() == EMPTY) || (table[ZobristKey % TTSize].GetDepth() < Depth) || (table[ZobristKey % TTSize].IsAncient()))
		table[ZobristKey % TTSize] = TTEntry(best, ZobristKey, Score, Depth, Cutoff);
}

TTEntry TranspositionTable::GetEntry(uint64_t key)
{
	TTHits++;
	table[key % TTSize].SetAncient(false);
	return table[key % TTSize];
}

void TranspositionTable::Reformat()
{
	for (int i = 0; i < TTSize; i++)
	{
		table[i].SetAncient(true);
	}
}
