#include "BoardParamiters.h"

BoardParamiters::BoardParamiters()
{
	
}

BoardParamiters::~BoardParamiters()
{
}

void BoardParamiters::WhiteCastled()
{
	m_WhiteKingCastle = false;
	m_WhiteQueenCastle = false;
	m_HasCastledWhite = true;
}

void BoardParamiters::BlackCastled()
{
	m_BlackKingCastle = false;
	m_BlackQueenCastle = false;
	m_HasCastledBlack = true;
}

void BoardParamiters::NextTurn()
{
	m_TurnCount++;
	m_CurrentTurn = !m_CurrentTurn;
}

void BoardParamiters::InitParamiters()
{
	PreviousParamiters.clear();

	m_CurrentTurn = WHITE;
	m_WhiteKingCastle = false;
	m_WhiteQueenCastle = false;
	m_BlackKingCastle = false;
	m_BlackQueenCastle = false;

	m_EnPassant = N_SQUARES;
	m_FiftyMoveCount = 0;
	m_TurnCount = 1;

	m_HasCastledWhite = false;
	m_HasCastledBlack = false;

	m_CaptureSquare = -1;
}

BoardParamiterData BoardParamiters::GetPreviousParamiters()
{
	assert(PreviousParamiters.size() != 0);
	return PreviousParamiters.back();
}

bool BoardParamiters::InitialiseParamitersFromFen(std::vector<std::string> fen)
{
	InitParamiters();

	if (fen[1] == "w")
		m_CurrentTurn = WHITE;

	if (fen[1] == "b")
		m_CurrentTurn = BLACK;

	if (fen[2].find('K') != std::string::npos)
		m_WhiteKingCastle = true;

	if (fen[2].find('Q') != std::string::npos)
		m_WhiteQueenCastle = true;

	if (fen[2].find('k') != std::string::npos)
		m_BlackKingCastle = true;

	if (fen[2].find('q') != std::string::npos)
		m_BlackQueenCastle = true;

	m_EnPassant = AlgebraicToPos(fen[3]);

	m_FiftyMoveCount = std::stoi(fen[4]);
	m_TurnCount = std::stoi(fen[5]);

	return true;
}

void BoardParamiters::SaveParamiters()
{
	PreviousParamiters.push_back(static_cast<BoardParamiterData>(*this));
}

void BoardParamiters::RestorePreviousParamiters()
{
	assert(PreviousParamiters.size() != 0);

	m_CaptureSquare = PreviousParamiters.back().GetCaptureSquare();
	m_EnPassant = PreviousParamiters.back().GetEnPassant();
	m_FiftyMoveCount = PreviousParamiters.back().GetFiftyMoveCount();
	m_TurnCount = PreviousParamiters.back().GetTurnCount();

	m_HasCastledWhite = PreviousParamiters.back().HasCastledWhite();
	m_HasCastledBlack = PreviousParamiters.back().HasCastledBlack();

	m_CurrentTurn = PreviousParamiters.back().GetTurn();
	m_WhiteKingCastle = PreviousParamiters.back().CanCastleWhiteKingside();
	m_WhiteQueenCastle = PreviousParamiters.back().CanCastleWhiteQueenside();
	m_BlackKingCastle = PreviousParamiters.back().CanCastleBlackKingside();
	m_BlackQueenCastle = PreviousParamiters.back().CanCastleBlackQueenside();

	PreviousParamiters.pop_back();			
}

void BoardParamiters::UpdateCastleRights(Move move)
{
	if (move.GetFrom() == SQ_E1 || move.GetTo() == SQ_E1)			//Check for the piece moving off the square, or a capture happening on the square (enemy moving to square)
	{
		m_WhiteKingCastle = false;
		m_WhiteQueenCastle = false;
	}

	if (move.GetFrom() == SQ_E8 || move.GetTo() == SQ_E8)
	{
		m_BlackKingCastle = false;
		m_BlackQueenCastle = false;
	}

	if (move.GetFrom() == SQ_A1 || move.GetTo() == SQ_A1)
	{
		m_WhiteQueenCastle = false;
	}

	if (move.GetFrom() == SQ_A8 || move.GetTo() == SQ_A8)
	{
		m_BlackQueenCastle = false;
	}

	if (move.GetFrom() == SQ_H1 || move.GetTo() == SQ_H1)
	{
		m_WhiteKingCastle = false;
	}

	if (move.GetFrom() == SQ_H8 || move.GetTo() == SQ_H8)
	{
		m_BlackKingCastle = false;
	}
}

BoardParamiterData::BoardParamiterData()
{
	m_CurrentTurn = WHITE;
	m_WhiteKingCastle = false;
	m_WhiteQueenCastle = false;
	m_BlackKingCastle = false;
	m_BlackQueenCastle = false;

	m_EnPassant = N_SQUARES;
	m_FiftyMoveCount = 0;
	m_TurnCount = 1;

	m_HasCastledWhite = false;
	m_HasCastledBlack = false;

	m_CaptureSquare = -1;
}
