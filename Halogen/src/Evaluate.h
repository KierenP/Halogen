#pragma once
#include <fstream>
#include <vector>
#include "PawnHashTable.h"
#include "PieceSquareTables.h"

const unsigned int PieceValues[N_PIECES] = { 100, 320, 330, 500, 900, 5000, 100, 320, 330, 500, 900, 5000 };

extern PawnHashTable pawnHashTable;

int EvaluatePosition(const Position& position);
void QueryPawnHashTable(const Position& position, int& pawns, unsigned int GameStage);
void InitializeEvaluation();

bool DeadPosition(const Position& position);
bool IsBlockade(const Position& position);

bool EvaluateDebug();


