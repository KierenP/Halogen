#pragma once
#include "BitBoardDefine.h"
#include <vector>

struct BitBoardData
{
	friend class BitBoard;	//so that bitboard can access m_Bitboard directly but the position class cannot 

	BitBoardData();

	//we need these functions because Zobrist increment will access the previous BitBoardData from previousBoards and it then needs to extract this information
	uint64_t GetPieceBB(unsigned int piece) const;
	unsigned int GetSquare(unsigned int square) const;	//returns N_PIECES = 12 if empty

private:
	uint64_t m_Bitboard[N_PIECES];
};

class BitBoard : public BitBoardData
{
public:
	BitBoard();
	virtual ~BitBoard() = 0;

	bool IsEmpty(unsigned int positon) const;
	bool IsOccupied(unsigned int position) const;
	bool IsOccupied(unsigned int position, bool colour) const;

	uint64_t GetAllPieces() const;
	uint64_t GetEmptySquares() const;
	uint64_t GetWhitePieces() const;
	uint64_t GetBlackPieces() const;
	uint64_t GetPiecesColour(bool colour) const;
	uint64_t GetPieceBB(unsigned int pieceType, bool colour) const;
	unsigned int GetKing(bool colour) const;

	void SetSquare(unsigned int square, unsigned int piece);
	void ClearSquare(unsigned int square);

	using BitBoardData::GetPieceBB;	//allow this function to be overloaded without it being overwriten

protected:
	void ResetBoard();
	bool InitialiseBoardFromFen(std::vector<std::string> fen);

	void SaveBoard();
	void RestorePreviousBoard();

	BitBoardData GetPreviousBoard();

private:
	std::vector<BitBoardData> previousBoards;
};

