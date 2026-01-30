#pragma once

#include <cstdint>

class BoardState;
enum File : int8_t;
enum Piece : int8_t;
enum Side : int8_t;
enum Square : int8_t;

namespace Zobrist
{
uint64_t piece_square(Piece piece, Square square) noexcept;
uint64_t stm() noexcept;
uint64_t en_passant(File file) noexcept;
uint64_t castle(Square square) noexcept;

uint64_t key(const BoardState& board) noexcept;
uint64_t pawn_key(const BoardState& board) noexcept;
uint64_t non_pawn_key(const BoardState& board, Side side) noexcept;

uint64_t get_fifty_move_adj_key(const BoardState& board) noexcept;
}
