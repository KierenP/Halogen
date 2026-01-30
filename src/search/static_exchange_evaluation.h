#pragma once

class BoardState;
class Move;
class Score;

int see(const BoardState& board, Move move) noexcept;
bool see_ge(const BoardState& board, Move move, Score threshold) noexcept;
