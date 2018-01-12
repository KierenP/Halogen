#pragma once
#include "BitBoardDefine.h"

class BitBoard
{
public:
	BitBoard();
	~BitBoard();

	void Reset();

	unsigned int GetSquare(unsigned int square);
	void SetSquare(unsigned int square, unsigned int piece);
	void ClearSquare(unsigned int square);

	bool Empty(unsigned int positon);
	bool Occupied(unsigned int position);
	bool Occupied(unsigned int position, unsigned int colour);

	uint64_t AllPieces();
	uint64_t EmptySquares();
	uint64_t WhitePieces();
	uint64_t BlackPieces();
	uint64_t ColourPieces(bool colour);

	uint64_t GetPieces(unsigned int piece);

	bool ColourOfPiece(unsigned int piece);

private:
	uint64_t m_Bitboard[N_PIECES];
};

