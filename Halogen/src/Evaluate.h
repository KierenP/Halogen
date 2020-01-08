#pragma once
#include <fstream>
#include <vector>
#include "PawnHashTable.h"
#include "PieceSquareTables.h"

extern PawnHashTable pawnHashTable;

int EvaluatePosition(const Position& position);
void InitializeEvaluation();
bool InsufficentMaterial(const Position& position, int Material);	//if you already have the material score -> faster!
bool InsufficentMaterial(const Position& position); //will count the material for you

bool EvaluateDebug();


