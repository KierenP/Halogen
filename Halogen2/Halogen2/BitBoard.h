#pragma once
#include "BitBoardDefine.h"
#include <vector>

class BitBoard;

extern std::vector<BitBoard> previousBoards;

class BitBoard
{
public:
	BitBoard();
	~BitBoard();

	unsigned int GetSquare(unsigned int square) const;	//returns N_PIECES = 12 if empty
	bool IsEmpty(unsigned int positon) const;
	bool IsOccupied(unsigned int position) const;
	bool IsOccupied(unsigned int position, bool colour) const;

	uint64_t GetAllPieces() const;
	uint64_t GetEmptySquares() const;
	uint64_t GetWhitePieces() const;
	uint64_t GetBlackPieces() const;
	uint64_t GetPiecesColour(bool colour) const;
	uint64_t GetPieceBB(unsigned int piece) const;
	uint64_t GetPieceBB(unsigned int pieceType, bool colour) const;
	unsigned int GetKing(bool colour) const;

	void SetSquare(unsigned int square, unsigned int piece);
	void ClearSquare(unsigned int square);

protected:
	void Reset();
	bool InitialiseBoardFromFen(std::vector<std::string> fen);

	void SaveBoard();
	void RestorePreviousBoard();

private:
	uint64_t m_Bitboard[N_PIECES];
};

