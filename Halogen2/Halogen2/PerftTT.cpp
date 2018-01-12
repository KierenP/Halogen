#include "PerftTT.h"

const unsigned int ZobristTableSize = 12 * 64 + 1 + 4 + 8;
uint64_t ZobristTable[ZobristTableSize];

PerftTTEntry::PerftTTEntry(uint64_t ZobristKey, unsigned int ChildNodes, unsigned int Depth)
{
	key = ZobristKey;
	nodes = ChildNodes;
	depth = Depth;
	ancient = false;
}

PerftTTEntry::~PerftTTEntry()
{
}

bool PerftTT::CheckEntry(uint64_t key)
{
	if (HashTable[key % HashTableSize].GetKey() == key)
		return true;
	return false;
}

void PerftTT::AddEntry(uint64_t key, unsigned childNodes, unsigned int depth)
{
	if (HashTable[key % HashTableSize].GetDepth() < depth || HashTable[key % HashTableSize].GetKey() == EMPTY || HashTable[key % HashTableSize].IsAncient())
		HashTable[key % HashTableSize] = PerftTTEntry(key, childNodes, depth);
}

PerftTTEntry PerftTT::GetEntry(uint64_t key)
{
	HashTable[key % HashTableSize].SetAncient(false);
	return HashTable[key % HashTableSize];
}

void PerftTT::Reformat()
{
	for (int i = 0; i < HashTableSize; i++)
	{
		HashTable[i].SetAncient(true);
	}
}

PerftTT::PerftTT()
{
	for (int i = 0; i < HashTableSize; i++)
	{
		HashTable[i] = PerftTTEntry();
	}
}


PerftTT::~PerftTT()
{
}

void ZobristInit()
{
	for (int i = 0; i < ZobristTableSize; i++)
	{
		ZobristTable[i] = genrand64_int64();
	}
}
