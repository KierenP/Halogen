#pragma once
#include "Position.h"
#include <functional>
#include <valarray>
#include <array>
#include <algorithm>

//needed for SEE
extern int pieceValueVector[N_STAGES][N_PIECE_TYPES];

bool DeadPosition(const Position& position);
bool IsBlockade(const Position& position);

int EvaluatePositionNet(Position& position, EvalCacheTable& evalTable);

int PieceValues(unsigned int Piece, GameStages GameStage = MIDGAME);

