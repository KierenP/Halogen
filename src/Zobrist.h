#pragma once
#include "BitBoardDefine.h"
#include <random>
#include <stdint.h>
#include <vector>

class Position;

class Zobrist
{
public:
    void Recalculate(const Position& position) { key = Generate(position); }
    uint64_t Key() const { return key; }

    void ToggleSTM() { key ^= ZobristTable[12 * 64]; }
    void ToggleWhiteKingside() { key ^= ZobristTable[12 * 64 + 1]; }
    void ToggleWhiteQueenside() { key ^= ZobristTable[12 * 64 + 2]; }
    void ToggleBlackKingsize() { key ^= ZobristTable[12 * 64 + 3]; }
    void ToggleBlackQueensize() { key ^= ZobristTable[12 * 64 + 4]; }
    void ToggleEnpassant(File file) { key ^= ZobristTable[12 * 64 + 5 + file]; }
    void TogglePieceSquare(Pieces piece, Square square) { key ^= ZobristTable[piece * 64 + square]; }

    bool Verify(const Position& position) const { return Generate(position) == key; }

private:
    static uint64_t Generate(const Position& position);

    // 12 pieces * 64 squares, 1 for side to move, 4 for casteling rights and 8 for ep squares
    const static std::array<uint64_t, 781> ZobristTable;

    uint64_t key = EMPTY;
};
