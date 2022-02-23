#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>

#include "BitBoardDefine.h"

class Move;
class Position;

template <typename T>
void LegalMoves(Position& position, T& moves);
template <typename T>
void QuiescenceMoves(Position& position, T& moves);
template <typename T>
void QuietMoves(Position& position, T& moves);

bool IsInCheck(const Position& position, Players colour);
bool IsInCheck(const Position& position);

bool MoveIsLegal(Position& position, const Move& move);

// Returns the attack bitboard for a piece of piecetype on square sq
template <PieceTypes pieceType>
uint64_t AttackBB(Square sq, uint64_t occupied = EMPTY);