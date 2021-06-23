#pragma once
#include <random>
#include <stdint.h>
#include <vector>

const unsigned int ZobristTableSize = 12 * 64 + 1 + 4 + 8;	//12 pieces * 64 squares, 1 for side to move, 4 for casteling rights and 8 for ep. square
extern std::vector<uint64_t> ZobristTable;
void ZobristInit();