#pragma once

#include <cstdint>

class BoardState;
class Move;
class Score;

constexpr int PieceValues[] = { 91, 532, 568, 715, 1279, 5000, 91, 532, 568, 715, 1279, 5000, 0 };

int see(const BoardState& board, Move move, uint64_t white_pinned, uint64_t black_pinned);
bool see_ge(const BoardState& board, Move move, Score threshold, uint64_t white_pinned, uint64_t black_pinned);
