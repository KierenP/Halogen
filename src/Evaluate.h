#pragma once

#include "Score.h"

class BoardState;
class Network;
struct SearchStackState;

bool DeadPosition(const BoardState& board);
Score Evaluate(const BoardState& board, SearchStackState* ss, Network& net);
