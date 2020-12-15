#include "BoardParamiters.h"

BoardParamiters::BoardParamiters()
{
	InitParamiters();
}

BoardParamiters::~BoardParamiters()
{
}

void BoardParamiters::WhiteCastled()
{
	SetCanCastleWhiteKingside(false);
	SetCanCastleWhiteQueenside(false);
	SetHasCastledWhite(true);
}

void BoardParamiters::BlackCastled()
{
	SetCanCastleBlackKingside(false);
	SetCanCastleBlackQueenside(false);
	SetHasCastledBlack(true);
}

void BoardParamiters::NextTurn()
{
	SetTurnCount(GetTurnCount() + 1);
	SetTurn(!GetTurn());
}

const BoardParamiterData& BoardParamiters::GetPreviousParamiters()
{
	size_t size = PreviousParamiters.size();
	return PreviousParamiters[size - 2];
}

void BoardParamiters::InitParamiters()
{
	PreviousParamiters = { BoardParamiterData() };
	Current = PreviousParamiters.begin();
}

bool BoardParamiters::InitialiseParamitersFromFen(std::vector<std::string> fen)
{
	InitParamiters();

	if (fen[1] == "w")
		SetTurn(WHITE);

	if (fen[1] == "b")
		SetTurn(BLACK);

	if (fen[2].find('K') != std::string::npos)
		SetCanCastleWhiteKingside(true);

	if (fen[2].find('Q') != std::string::npos)
		SetCanCastleWhiteQueenside(true);

	if (fen[2].find('k') != std::string::npos)
		SetCanCastleBlackKingside(true);

	if (fen[2].find('q') != std::string::npos)
		SetCanCastleBlackQueenside(true);

	SetEnPassant(static_cast<Square>(AlgebraicToPos(fen[3])));

	SetFiftyMoveCount(std::stoi(fen[4]));
	SetTurnCount(std::stoi(fen[5]));

	return true;
}

void BoardParamiters::SaveParamiters()
{
	PreviousParamiters.emplace_back(*Current);
	Current = --PreviousParamiters.end();		//Current might be invalidated by the above line if reallocation occurs
}

void BoardParamiters::RestorePreviousParamiters()
{
	assert(PreviousParamiters.size() != 0);

	Current--;
	PreviousParamiters.pop_back();	//the iterator should never be invalitated by this
}

void BoardParamiters::UpdateCastleRights(Move move)
{
	if (move.GetFrom() == SQ_E1 || move.GetTo() == SQ_E1)			//Check for the piece moving off the square, or a capture happening on the square (enemy moving to square)
	{
		SetCanCastleWhiteKingside(false);
		SetCanCastleWhiteQueenside(false);
	}

	if (move.GetFrom() == SQ_E8 || move.GetTo() == SQ_E8)
	{
		SetCanCastleBlackKingside(false);
		SetCanCastleBlackQueenside(false);
	}

	if (move.GetFrom() == SQ_A1 || move.GetTo() == SQ_A1)
	{
		SetCanCastleWhiteQueenside(false);
	}

	if (move.GetFrom() == SQ_A8 || move.GetTo() == SQ_A8)
	{
		SetCanCastleBlackQueenside(false);
	}

	if (move.GetFrom() == SQ_H1 || move.GetTo() == SQ_H1)
	{
		SetCanCastleWhiteKingside(false);
	}

	if (move.GetFrom() == SQ_H8 || move.GetTo() == SQ_H8)
	{
		SetCanCastleBlackKingside(false);
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

	m_CaptureSquare = N_SQUARES;		
	m_CapturePiece = N_PIECES;
}
