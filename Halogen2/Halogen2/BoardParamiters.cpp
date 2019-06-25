#include "BoardParamiters.h"

std::vector<BoardParamiters> PreviousParamiters;

BoardParamiters::BoardParamiters()
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

bool BoardParamiters::InitialiseParamitersFromFen(std::vector<std::string> fen)
{
	m_WhiteKingCastle = false;
	m_WhiteQueenCastle = false;
	m_BlackKingCastle = false;
	m_BlackQueenCastle = false;

	m_HasCastledWhite = false;
	m_HasCastledBlack = false;

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
	PreviousParamiters.push_back(*this);
}

void BoardParamiters::RestorePreviousParamiters()
{
	if (PreviousParamiters.size() == 0) throw std::invalid_argument("Yeet");

	*this = PreviousParamiters[PreviousParamiters.size() - 1];
	PreviousParamiters.erase(PreviousParamiters.end() - 1);			//erase the last element
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
