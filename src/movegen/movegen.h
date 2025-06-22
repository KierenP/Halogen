#pragma once
#include <cstdint>

#include "bitboard/define.h"

class BoardState;
class Move;

template <typename T>
void LegalMoves(const BoardState& board, T& moves);
template <typename T>
void QuiescenceMoves(const BoardState& board, T& moves);
template <typename T>
void QuietMoves(const BoardState& board, T& moves);

bool IsInCheck(const BoardState& board, Side colour);
bool IsInCheck(const BoardState& board);

bool MoveIsLegal(const BoardState& board, const Move& move);
bool EnPassantIsLegal(const BoardState& board, const Move& move);

// Returns a threat mask that only contains winning captures (RxQ, B/NxR/Q, Px!P)
uint64_t ThreatMask(const BoardState& board, Side colour);

// Returns the attack bitboard for a piece of piecetype on square sq
template <PieceType pieceType>
uint64_t AttackBB(Square sq, uint64_t occupied = EMPTY);