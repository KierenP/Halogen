#pragma once

#include "search/score.h"

class BoardState;
struct SearchStackState;

namespace NN
{
class Network;
class Accumulator;
}

bool insufficient_material(const BoardState& board);
Score evaluate(const BoardState& board, NN::Accumulator* acc, NN::Network& net);
