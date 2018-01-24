#pragma once
#include "TranspositionTable.h"
#include "ABnode.h"
#include "Position.h"
#include "MoveGeneration.h"
#include "Zobrist.h"
#include "Evaluate.h"
#include "Move.h"
#include <math.h>

extern TranspositionTable tTable;

void OrderMoves(std::vector<Move>& moves, Position & position, int searchDepth);
Move SearchPosition(Position & position, int allowedTimeMs, bool printInfo);
