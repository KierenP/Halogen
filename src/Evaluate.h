#pragma once

#include "Score.h"

class BoardState;
struct SearchStackState;

namespace NN
{
class Network;
}

bool DeadPosition(const BoardState& board);
Score Evaluate(const BoardState& board, SearchStackState* ss, NN::Network& net);
