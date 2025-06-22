#pragma once

#include "Score.h"

class BoardState;
struct SearchStackState;

namespace NN
{
class Network;
}

bool insufficient_material(const BoardState& board);
Score evaluate(const BoardState& board, SearchStackState* ss, NN::Network& net);
