#pragma once

#include "GameState.h"
#include "Zobrist.h"

class EvalCacheTable;
class BoardState;
class GameState;

bool DeadPosition(const BoardState& board);
int EvaluatePositionNet(const GameState& position, EvalCacheTable& evalTable);
