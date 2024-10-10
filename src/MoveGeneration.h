#pragma once
#include <cstdint>

#include "BitBoardDefine.h"
#include "Bitboard.h"
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

// This function does not work for casteling moves. They are tested for legality their creation.
bool MovePutsSelfInCheck(const BoardState& board, const Move& move);

// Returns a threat mask that only contains winning captures (RxQ, B/NxR/Q, Px!P)
BB ThreatMask(const BoardState& board, Players colour);

// Returns the attack bitboard for a piece of piecetype on square sq
template <PieceTypes pieceType>
BB AttackBB(Square sq, BB occupied = BB::none);