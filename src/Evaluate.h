#pragma once

#include "Score.h"

class BoardState;
class GameState;
class Network;
struct Accumulator;
class SearchStackState;

bool DeadPosition(const BoardState& board);
Score Evaluate(const BoardState& board, SearchStackState* ss, Network& net);
