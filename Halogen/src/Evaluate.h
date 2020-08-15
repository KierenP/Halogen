#pragma once
#include <fstream>
#include <vector>
#include "PawnHashTable.h"
#include "PieceSquareTables.h"

int PieceValues(unsigned int Piece, GameStages GameStage = GameStages::MIDGAME);

extern PawnHashTable pawnHashTable;

int EvaluatePosition(const Position& position);
void QueryPawnHashTable(const Position& position, int& pawns, GameStages GameStage);
void EvalInit();

bool DeadPosition(const Position& position);
bool IsBlockade(const Position& position);

bool EvaluateDebug();

std::vector<int*> TexelParamiters();
std::vector<int*> TexelPST();


