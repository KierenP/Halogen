#pragma once
#include <fstream>
#include <vector>
#include "PawnHashTable.h"
#include "PieceSquareTables.h"

extern PawnHashTable pawnHashTable;

int EvaluatePosition(const Position& position);
void QueryPawnHashTable(const Position& position, int& pawns, unsigned int GameStage);
void InitializeEvaluation();

bool DeadPosition(const Position& position);

bool EvaluateDebug();


