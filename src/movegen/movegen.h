#pragma once

#include <cstdint>

#include "bitboard/define.h"

enum PieceType : int8_t;
enum Square : int8_t;

class BoardState;
class Move;

template <typename T>
void legal_moves(const BoardState& board, T& moves) noexcept;
template <typename T>
void loud_moves(const BoardState& board, T& moves) noexcept;
template <typename T>
void quiet_moves(const BoardState& board, T& moves) noexcept;

bool is_legal(const BoardState& board, const Move& move) noexcept;
bool ep_is_legal(const BoardState& board, const Move& move) noexcept;

// Returns the attack bitboard for a piece of piecetype on square sq
template <PieceType pieceType>
uint64_t attack_bb(Square sq, uint64_t occupied = EMPTY) noexcept;