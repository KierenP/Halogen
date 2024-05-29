#pragma once
#include <cstdint>

#include "BitBoardDefine.h"
#include "Zobrist.h"

class Move;
class BoardState;

// Get all legal moves in a position
template <typename T>
void LegalMoves(const BoardState& board, T& moves);

// Generate captures and promotions
template <typename T>
void LoudMoves(const BoardState& board, T& moves);

// Generate captures and promotions. Uses special generators because we are in check
template <typename T>
void LoudMovesInCheck(const BoardState& board, T& moves);

// Generate quiet moves
template <typename T>
void QuietMoves(const BoardState& board, T& moves);

// Generate quiet moves
template <typename T>
void QuietMovesInCheck(const BoardState& board, T& moves);

bool IsInCheck(const BoardState& board, Players colour);
bool IsInCheck(const BoardState& board);
bool MoveIsLegal(const BoardState& board, const Move& move);

// Returns the attack bitboard for a piece of piecetype on square sq
template <PieceTypes pieceType>
uint64_t AttackBB(Square sq, uint64_t occupied = EMPTY);