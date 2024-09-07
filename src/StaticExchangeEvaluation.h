#pragma once

class BoardState;
class Move;

constexpr int PieceValues[] = { 91, 532, 568, 715, 1279, 5000, 91, 532, 568, 715, 1279, 5000 };

int see(const BoardState& board, Move move);
