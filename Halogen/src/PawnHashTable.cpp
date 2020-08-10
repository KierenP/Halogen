#include "PawnHashTable.h"

PawnHashTable::PawnHashTable()
{
	table.push_back(PawnHash());
}

PawnHashTable::~PawnHashTable()
{
}

void PawnHashTable::ResetTable()
{
	HashHits = 0;
	HashMisses = 0;

	for (size_t i = 0; i < table.size(); i++)
	{
		table[i] = PawnHash();
	}
}

void PawnHashTable::Init(unsigned int size)
{
	table.clear();
	unsigned int EntrySize = sizeof(PawnHash);
	unsigned int entires = size * 1024 * 1024 / EntrySize;

	for (size_t i = 0; i < entires; i++)
	{
		table.push_back(PawnHash());
	}
}

bool PawnHashTable::CheckEntry(uint64_t key)
{
	if ((table.at(key % table.size()).key == key) && table.at(key % table.size()).occupied)
		return true;
	return false;
}

PawnHash PawnHashTable::GetEntry(uint64_t key)
{
	return table.at(key % table.size());
}

void PawnHashTable::AddEntry(uint64_t hashKey, int score)
{
	if (!HASH_ENABLE)
		return;

	table.at(hashKey % table.size()) = PawnHash(hashKey, score, true);
}

uint64_t PawnHashKey(const Position& pos, char gameStage)
{
	uint64_t Key = EMPTY;

	uint64_t bitboard = pos.GetPieceBB(WHITE_PAWN);
	while (bitboard != 0)
	{
		Key ^= ZobristTable.at(WHITE_PAWN * 64 + bitScanForwardErase(bitboard));
	}

	bitboard = pos.GetPieceBB(BLACK_PAWN);
	while (bitboard != 0)
	{
		Key ^= ZobristTable.at(BLACK_PAWN * 64 + bitScanForwardErase(bitboard));
	}

	if (gameStage == MIDGAME)
		Key ^= ZobristTable.at(0);
	if (gameStage == ENDGAME)
		Key ^= ZobristTable.at(1);

	return Key;
}
