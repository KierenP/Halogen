#pragma once
#include <array>
#include <cstdint>

#include "BitBoardDefine.h"

class BoardState;

class Zobrist
{
public:
    void Recalculate(const BoardState& board)
    {
        key = Generate(board);
    }

    uint64_t Key() const
    {
        return key;
    }

    void ToggleSTM()
    {
        key ^= ZobristTable[12 * 64];
    }

    void ToggleCastle(Square sq);

    void ToggleEnpassant(File file)
    {
        key ^= ZobristTable[12 * 64 + 17 + file];
    }

    void TogglePieceSquare(Pieces piece, Square square)
    {
        key ^= ZobristTable[piece * 64 + square];
    }

    bool Verify(const BoardState& board) const
    {
        return Generate(board) == key;
    }

private:
    static uint64_t Generate(const BoardState& board);

    // 12 pieces * 64 squares, 1 for side to move, 16 for casteling rights and 8 for ep squares
    const static std::array<uint64_t, 12 * 64 + 1 + 16 + 8> ZobristTable;

    uint64_t key = 0;
};

class PawnKey
{
public:
    void recalculate(const BoardState& board)
    {
        key = generate(board);
    }

    operator uint64_t() const
    {
        return key;
    }

    void toggle_piece_square(Pieces piece, Square square)
    {
        key ^= table[((piece == WHITE_PAWN) ? 0 : 64) + square];
    }

    bool verify(const BoardState& board) const
    {
        return generate(board) == key;
    }

private:
    static uint64_t generate(const BoardState& board);

    // 2 pieces * 64 squares
    const static std::array<uint64_t, 2 * 64> table;
    uint64_t key = 0;
};
