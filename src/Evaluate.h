#pragma once

#include "GameState.h"
#include "Score.h"
#include "Zobrist.h"

class BoardState;
class GameState;

bool DeadPosition(const BoardState& board);
Score Evaluate(const GameState& position);
