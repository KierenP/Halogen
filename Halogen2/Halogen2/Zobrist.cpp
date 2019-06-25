#include "Zobrist.h"

std::vector<uint64_t> ZobristTable;

void ZobristInit()
{
	for (int i = 0; i < ZobristTableSize; i++)
	{
		ZobristTable.push_back(genrand64_int64());
	}
}
