#pragma once
#include <cstdint>

#include "bitboard/define.h"

class BoardState;

namespace Zobrist
{
uint64_t piece_square(Piece piece, Square square);
uint64_t stm();
uint64_t en_passant(File file);
uint64_t castle(Square square);

uint64_t key(const BoardState& board);
uint64_t pawn_key(const BoardState& board);
uint64_t non_pawn_key(const BoardState& board, Side side);

uint64_t get_fifty_move_adj_key(const BoardState& board);
}
