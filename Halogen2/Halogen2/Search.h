#pragma once
#include "TranspositionTable.h"
#include "ABnode.h"
#include "Position.h"
#include "MoveGeneration.h"
#include "Zobrist.h"
#include "Evaluate.h"
#include "Move.h"
#include <ctime>
#include <algorithm>

extern TranspositionTable tTable;

void OrderMoves(std::vector<Move>& moves, Position & position, int searchDepth);
Move SearchPosition(Position & position, int allowedTimeMs);
std::vector<Move> SearchBenchmark(Position& position, int allowedTimeMs);
