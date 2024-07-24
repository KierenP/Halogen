#pragma once

#include "Score.h"

class BoardState;
class GameState;
class Network;
struct Accumulator;

bool DeadPosition(const BoardState& board);
Score Evaluate(const BoardState& board, const Accumulator& acc);
