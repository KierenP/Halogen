#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>

#include "BitBoardDefine.h"
#include "Zobrist.h"

class Move;
class BoardState;

template <typename T>
void LegalMoves(const BoardState& board, T& moves);
template <typename T>
void QuiescenceMoves(const BoardState& board, T& moves);
template <typename T>
void QuietMoves(const BoardState& board, T& moves);

bool IsInCheck(const BoardState& board, Players colour);
bool IsInCheck(const BoardState& board);

bool MoveIsLegal(const BoardState& board, const Move& move);

// Returns the attack bitboard for a piece of piecetype on square sq
template <PieceTypes pieceType>
uint64_t AttackBB(Square sq, uint64_t occupied = EMPTY);