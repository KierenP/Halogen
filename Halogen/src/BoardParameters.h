#pragma once
#include "BitBoardDefine.h"
#include "Move.h"
#include <vector>

struct BoardParameterData
{
	BoardParameterData();

	Square m_CaptureSquare;
	Pieces m_CapturePiece;
	Square m_EnPassant;
	unsigned int m_FiftyMoveCount;
	unsigned int m_TurnCount;

	bool m_HasCastledWhite;
	bool m_HasCastledBlack;

	Players m_CurrentTurn;
	bool m_WhiteKingCastle;
	bool m_WhiteQueenCastle;
	bool m_BlackKingCastle;
	bool m_BlackQueenCastle;
};

class BoardParameters
{
public:
	BoardParameters();
	virtual ~BoardParameters() = 0;

	unsigned int GetTurnCount() const { return Current->m_TurnCount; }
	Players GetTurn() const { return Current->m_CurrentTurn; }
	bool GetHasCastledWhite() const { return Current->m_HasCastledWhite; }
	bool GetHasCastledBlack() const { return Current->m_HasCastledBlack; }
	bool GetCanCastleWhiteKingside() const { return Current->m_WhiteKingCastle; }
	bool GetCanCastleWhiteQueenside() const { return Current->m_WhiteQueenCastle; }
	bool GetCanCastleBlackKingside() const { return Current->m_BlackKingCastle; }
	bool GetCanCastleBlackQueenside() const { return Current->m_BlackQueenCastle; }
	Square GetEnPassant() const { return Current->m_EnPassant; }
	Square GetCaptureSquare() const { return Current->m_CaptureSquare; }
	Pieces GetCapturePiece() const { return Current->m_CapturePiece; }
	unsigned int GetFiftyMoveCount() const { return Current->m_FiftyMoveCount; }

	void SetTurnCount(unsigned int val) { Current->m_TurnCount = val; }
	void SetTurn(Players val) { Current->m_CurrentTurn = val; }
	void SetHasCastledWhite(bool val) { Current->m_HasCastledWhite = val; }
	void SetHasCastledBlack(bool val) { Current->m_HasCastledBlack = val; }
	void SetCanCastleWhiteKingside(bool val) { Current->m_WhiteKingCastle = val; }
	void SetCanCastleWhiteQueenside(bool val) { Current->m_WhiteQueenCastle = val; }
	void SetCanCastleBlackKingside(bool val) { Current->m_BlackKingCastle = val; }
	void SetCanCastleBlackQueenside(bool val) { Current->m_BlackQueenCastle = val; }
	void SetEnPassant(Square val) { Current->m_EnPassant = val; }
	void SetCaptureSquare(Square val) { Current->m_CaptureSquare = val; }
	void SetCapturePiece(Pieces val) { Current->m_CapturePiece = val; }
	void SetFiftyMoveCount(unsigned int val) { Current->m_FiftyMoveCount = val; }

protected:

	//Functions for the Zobrist key incremental updates
	Square PrevGetEnPassant() const { return (Current - 1)->m_EnPassant; }
	bool PrevGetCanCastleWhiteKingside() const { return (Current - 1)->m_WhiteKingCastle; }
	bool PrevGetCanCastleWhiteQueenside() const { return (Current - 1)->m_WhiteQueenCastle; }
	bool PrevGetCanCastleBlackKingside() const { return (Current - 1)->m_BlackKingCastle; }
	bool PrevGetCanCastleBlackQueenside() const { return (Current - 1)->m_BlackQueenCastle; }

	bool InitialiseParametersFromFen(std::vector<std::string> fen);
	void SaveParameters();
	void RestorePreviousParameters();
	void UpdateCastleRights(Move move);
	void WhiteCastled();
	void BlackCastled();
	void NextTurn();
	void Increment50Move() { SetFiftyMoveCount(GetFiftyMoveCount() + 1); }
	void Reset50Move() { SetFiftyMoveCount(0); }

	//The only function like this, because we need to be able to do this when detecting 50 move repititions
	unsigned int GetPreviousFiftyMove(unsigned int index) const { return PreviousParameters.at(index).m_FiftyMoveCount; }

	void InitParameters();

private:

	std::vector<BoardParameterData> PreviousParameters = { BoardParameterData() };
	std::vector<BoardParameterData>::iterator Current = PreviousParameters.begin();
};

