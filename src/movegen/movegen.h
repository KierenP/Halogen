#pragma once
#include <cstdint>

#include "bitboard/define.h"

class BoardState;
class Move;

template <typename T>
void legal_moves(const BoardState& board, T& moves);
template <typename T>
void loud_moves(const BoardState& board, T& moves);
template <typename T>
void quiet_moves(const BoardState& board, T& moves);

bool is_in_check(const BoardState& board, Side colour);
bool is_in_check(const BoardState& board);

bool is_legal(const BoardState& board, const Move& move);
bool ep_is_legal(const BoardState& board, const Move& move);

// Returns a threat mask that only contains winning captures (RxQ, B/NxR/Q, Px!P)
uint64_t capture_threat_mask(const BoardState& board, Side colour);

// Returns the attack bitboard for a piece of piecetype on square sq
template <PieceType pieceType>
uint64_t attack_bb(Square sq, uint64_t occupied = EMPTY);