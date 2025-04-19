#pragma once

class BoardState;
class Move;
class Score;

constexpr int PieceValues[] = { 91, 532, 568, 715, 1279, 5000, 91, 532, 568, 715, 1279, 5000, 0 };

int see(const BoardState& board, Move move);
bool see_ge(const BoardState& board, Move move, Score threshold);
