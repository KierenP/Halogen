#pragma once
#include "Zobrist.h"

const unsigned int HashTableSize = 1000000;//67108864;			//2^26 * 16 bytes = 2^30 bytes = 1GB

class PerftTTEntry
{
public:
	PerftTTEntry(uint64_t ZobristKey = EMPTY, unsigned int ChildNodes = -1, unsigned int Depth = -1);
	~PerftTTEntry();

	uint64_t GetKey() { return key; }
	unsigned int GetNodes() { return nodes; }
	unsigned int GetDepth() { return depth; }
	bool IsAncient() { return ancient; }

	void SetKey(uint64_t Key) { key = Key; }
	void SetAncient(bool isAncient) { ancient = isAncient; }

private:
	uint64_t key;			//8 bytes
	unsigned int nodes;		//4 bytes
	char depth;				//1 bytes
	bool ancient;			//1 byte = 14 total but for some reason it complies to 16 bytes. Maybe some padding?
};

class PerftTT
{
public:
	PerftTT();
	~PerftTT();

	bool CheckEntry(uint64_t key);
	void AddEntry(uint64_t key, unsigned childNodes, unsigned int depth);
	PerftTTEntry GetEntry(uint64_t key);

	void Reformat();

private:
	PerftTTEntry HashTable[HashTableSize];
};

