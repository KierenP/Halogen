#pragma once
#include "Position.h"
#include "weights.h"



#include <functional>
#include <valarray>
#include <array>

//needed for SEE
extern int pieceValueVector[N_STAGES][N_PIECE_TYPES];

bool DeadPosition(const Position& position);
bool IsBlockade(const Position& position);

int EvaluatePositionNet(Position& position);

int PieceValues(unsigned int Piece, GameStages GameStage = MIDGAME);

