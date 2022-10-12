#pragma once
#include <array>
#include <cstdint>

#include "BitBoardDefine.h"

class BoardState;

class Zobrist
{
public:
    void Recalculate(const BoardState& board) { key = Generate(board); }
    uint64_t Key() const { return key; }

    void ToggleSTM() { key ^= ZobristTable[12 * 64]; }
    void ToggleWhiteKingside() { key ^= ZobristTable[12 * 64 + 1]; }
    void ToggleWhiteQueenside() { key ^= ZobristTable[12 * 64 + 2]; }
    void ToggleBlackKingsize() { key ^= ZobristTable[12 * 64 + 3]; }
    void ToggleBlackQueensize() { key ^= ZobristTable[12 * 64 + 4]; }
    void ToggleEnpassant(File file) { key ^= ZobristTable[12 * 64 + 5 + file]; }
    void TogglePieceSquare(Pieces piece, Square square) { key ^= ZobristTable[piece * 64 + square]; }

    bool Verify(const BoardState& board) const { return Generate(board) == key; }

private:
    static uint64_t Generate(const BoardState& board);

    // 12 pieces * 64 squares, 1 for side to move, 4 for casteling rights and 8 for ep squares
    const static std::array<uint64_t, 781> ZobristTable;

    uint64_t key = EMPTY;
};
