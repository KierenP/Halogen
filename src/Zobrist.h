#pragma once
#include <array>
#include <cstdint>

#include "BitBoardDefine.h"

class BoardState;

namespace Zobrist
{
uint64_t piece_square(Pieces piece, Square square);
uint64_t stm();
uint64_t en_passant(File file);
uint64_t castle(Square square);

uint64_t key(const BoardState& board);
uint64_t pawn_key(const BoardState& board);
uint64_t non_pawn_key(const BoardState& board, Players side);
uint64_t minor_key(const BoardState& board);
uint64_t major_key(const BoardState& board);
}
