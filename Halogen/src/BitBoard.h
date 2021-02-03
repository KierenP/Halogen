#pragma once
#include "BitBoardDefine.h"
#include <vector>

struct BitBoardData
{
	BitBoardData();
	uint64_t m_Bitboard[N_PIECES];
};

class BitBoard
{
public:
	BitBoard();
	virtual ~BitBoard() = 0;

	Pieces GetSquare(Square square) const;

	bool IsEmpty(Square square) const;
	bool IsOccupied(Square square) const;
	bool IsOccupied(Square square, Players colour) const;

	uint64_t GetAllPieces() const;
	uint64_t GetEmptySquares() const;
	uint64_t GetWhitePieces() const;
	uint64_t GetBlackPieces() const;
	uint64_t GetPiecesColour(Players colour) const;
	uint64_t GetPieceBB(PieceTypes pieceType, Players colour) const;
	uint64_t GetPieceBB(Pieces piece) const;

	Square GetKing(Players colour) const;

	void SetSquare(Square square, Pieces piece);
	void ClearSquare(Square square);

protected:
	void ResetBoard();
	bool InitialiseBoardFromFen(const std::vector<std::string> &fen);

	void SaveBoard();
	void RestorePreviousBoard();

private:
	std::vector<BitBoardData> previousBoards = { BitBoardData() };
	std::vector<BitBoardData>::iterator Current = previousBoards.begin();
};

