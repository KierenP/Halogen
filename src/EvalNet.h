#pragma once

#include "GameState.h"
#include "Score.h"
#include "Zobrist.h"

class EvalCacheTable;
class BoardState;
class GameState;

bool DeadPosition(const BoardState& board);
Score EvaluatePositionNet(const GameState& position, EvalCacheTable& evalTable);
