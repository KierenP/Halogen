#pragma once

#include "spsa/tuneable.h"
#include <array>

class BoardState;
class Move;
class Score;

int see(const BoardState& board, Move move);
bool see_ge(const BoardState& board, Move move, Score threshold);
