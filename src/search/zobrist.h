#pragma once

#include <cstdint>

class BoardState;
class Move;
enum File : int8_t;
enum Piece : int8_t;
enum Side : int8_t;
enum Square : int8_t;

namespace Zobrist
{
uint64_t piece_square(Piece piece, Square square);
uint64_t stm();
uint64_t en_passant(File file);
uint64_t castle(Square square);

uint64_t key(const BoardState& board);
uint64_t pawn_key(const BoardState& board);
uint64_t non_pawn_key(const BoardState& board, Side side);

uint64_t update_castle_rights(uint64_t& castle_squares, Square white_king, Square black_king, Move move);

uint64_t get_fifty_move_adj_key(const BoardState& board);

// The fifty move adjusted key of the position that will result from playing 'move' on 'board', without applying the
// move. Used to prefetch the TT entry for a child node before apply_move, giving the prefetch more time to complete.
uint64_t get_fifty_move_adj_key_after(const BoardState& board, Move move);
}
