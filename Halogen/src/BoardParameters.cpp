#include "BoardParameters.h"

BoardParameters::~BoardParameters() = default;

void BoardParameters::WhiteCastled()
{
	SetCanCastleWhiteKingside(false);
	SetCanCastleWhiteQueenside(false);
	SetHasCastledWhite(true);
}

void BoardParameters::BlackCastled()
{
	SetCanCastleBlackKingside(false);
	SetCanCastleBlackQueenside(false);
	SetHasCastledBlack(true);
}

void BoardParameters::NextTurn()
{
	SetTurnCount(GetTurnCount() + 1);
	SetTurn(!GetTurn());
}

void BoardParameters::InitParameters()
{
	PreviousParameters = { BoardParameterData() };
	Current = PreviousParameters.begin();
}

bool BoardParameters::InitialiseParametersFromFen(const std::vector<std::string> &fen)
{
	InitParameters();

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

void BoardParameters::SaveParameters()
{
	PreviousParameters.emplace_back(*Current);
	Current = --PreviousParameters.end();		//Current might be invalidated by the above line if reallocation occurs
}

void BoardParameters::RestorePreviousParameters()
{
	assert(PreviousParameters.size() != 0);

	--Current;
	PreviousParameters.pop_back();	//the iterator should never be invalitated by this
}

void BoardParameters::UpdateCastleRights(Move move)
{
	if (GetFrom(move) == SQ_E1 || GetTo(move) == SQ_E1)			//Check for the piece moving off the square, or a capture happening on the square (enemy moving to square)
	{
		SetCanCastleWhiteKingside(false);
		SetCanCastleWhiteQueenside(false);
	}

	if (GetFrom(move) == SQ_E8 || GetTo(move) == SQ_E8)
	{
		SetCanCastleBlackKingside(false);
		SetCanCastleBlackQueenside(false);
	}

	if (GetFrom(move) == SQ_A1 || GetTo(move) == SQ_A1)
	{
		SetCanCastleWhiteQueenside(false);
	}

	if (GetFrom(move) == SQ_A8 || GetTo(move) == SQ_A8)
	{
		SetCanCastleBlackQueenside(false);
	}

	if (GetFrom(move) == SQ_H1 || GetTo(move) == SQ_H1)
	{
		SetCanCastleWhiteKingside(false);
	}

	if (GetFrom(move) == SQ_H8 || GetTo(move) == SQ_H8)
	{
		SetCanCastleBlackKingside(false);
	}
}
