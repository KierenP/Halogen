#pragma once
#include "TranspositionTable.h"
#include "ABnode.h"
#include "Position.h"
#include "MoveGeneration.h"
#include "Zobrist.h"
#include "Evaluate.h"

extern TranspositionTable tTable;

void OrderMoves(std::vector<Move>& moves, Position & position, int searchDepth);

ABnode SearchBestMove(Position & position, int depth, Move prevBest = Move());
