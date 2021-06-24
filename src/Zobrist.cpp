#include "Zobrist.h"

void ZobristInit()
{
	std::mt19937_64 gen(0);
	std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

	for (unsigned int i = 0; i < ZobristTableSize; i++)
	{
		ZobristTable.push_back(dist(gen));
	}
}
