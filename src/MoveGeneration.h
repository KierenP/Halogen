#pragma once
#include <cstdint>

#include "BitBoardDefine.h"
#include "Zobrist.h"

class Move;
class BoardState;

// For simplicity, the 'checks' generators only generate regular checks (no discovered checks) and skip pawn moves. Pawn
// and discovered checks are instead included in the 'non-checks' generator.
enum Generator
{
    MOVES_LEGAL,
    MOVES_LOUD_NON_CHECKS,
    MOVES_LOUD_CHECKS,
    MOVES_QUIET_NON_CHECKS,
    MOVES_QUIET_CHECKS,
};

template <Generator type, typename T>
void GenerateMoves(const BoardState& board, T& moves);

bool IsInCheck(const BoardState& board, Players colour);
bool IsInCheck(const BoardState& board);
bool MoveIsLegal(const BoardState& board, const Move& move);

// Returns the attack bitboard for a piece of piecetype on square sq
template <PieceTypes pieceType>
uint64_t AttackBB(Square sq, uint64_t occupied = EMPTY);