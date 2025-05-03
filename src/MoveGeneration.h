#pragma once
#include <cstdint>

#include "BitBoardDefine.h"

class BoardState;
class Move;

template <typename T>
void LegalMoves(const BoardState& board, T& moves);
template <typename T>
void QuiescenceMoves(const BoardState& board, T& moves);
template <typename T>
void QuietMoves(const BoardState& board, T& moves);

bool IsInCheck(const BoardState& board, Players colour);
bool IsInCheck(const BoardState& board);

bool MoveIsPsudolegal(const BoardState& board, const Move& move, uint64_t checkers);
bool MoveIsLegal(const BoardState& board, const Move& move, uint64_t pinned);
bool EnPassantIsLegal(const BoardState& board, const Move& move);

// Returns a threat mask that only contains winning captures (RxQ, B/NxR/Q, Px!P)
uint64_t ThreatMask(const BoardState& board, Players colour);

uint64_t Checkers(const BoardState& board);
uint64_t PinnedMask(const BoardState& board, Players side);

// Returns the attack bitboard for a piece of piecetype on square sq
template <PieceTypes pieceType>
uint64_t AttackBB(Square sq, uint64_t occupied = EMPTY);