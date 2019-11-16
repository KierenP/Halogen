#pragma once
#include <fstream>
#include <vector>
#include "Position.h"
#include "PieceSquareTables.h"

int EvaluatePosition(const Position& position);
void InitializeEvaluation();

bool EvaluateDebug();


