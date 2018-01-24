#pragma once
#include "Position.h"
#include "Random.h"

const unsigned int ZobristTableSize = 12 * 64 + 1 + 4 + 8;
extern uint64_t ZobristTable[ZobristTableSize];

uint64_t GenerateZobristKey(Position& position);
void ZobristInit();