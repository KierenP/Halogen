#include "Zobrist.h"

uint64_t ZobristTable[ZobristTableSize];

uint64_t GenerateZobristKey(Position & position)
{
	uint64_t Key = EMPTY;

	for (int i = 0; i < N_PIECES; i++)
	{
		uint64_t bitboard = position.GetPieceBB(i);
		while (bitboard != 0)
		{
			Key ^= ZobristTable[i * 64 + bitScanFowardErase(bitboard)];
		}
	}

	if (position.GetTurn() == WHITE)
		Key ^= ZobristTable[12 * 64];

	if (position.CanCastleWhiteKingside())
		Key ^= ZobristTable[12 * 64 + 1];
	if (position.CanCastleWhiteQueenside())
		Key ^= ZobristTable[12 * 64 + 2];
	if (position.CanCastleBlackKingside())
		Key ^= ZobristTable[12 * 64 + 3];
	if (position.CanCastleBlackQueenside())
		Key ^= ZobristTable[12 * 64 + 4];

	if (position.GetEnPassant() <= SQ_H8)														//no ep = -1 which wraps around to a very large number
	{
		Key ^= ZobristTable[12 * 64 + 5 + GetFile(position.GetEnPassant())];
	}

	return Key;
}

void ZobristInit()
{
	for (int i = 0; i < ZobristTableSize; i++)
	{
		ZobristTable[i] = genrand64_int64();
	}
}
