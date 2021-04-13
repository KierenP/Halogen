#include "BitBoard.h"
#include <stdexcept>

BitBoard::~BitBoard() = default;

void BitBoard::ResetBoard()
{
	previousBoards = { BitBoardData() };
	RecalculateWhiteBlackBoards();
}

bool BitBoard::InitialiseBoardFromFen(const std::vector<std::string> &fen)
{
	ResetBoard();

	size_t FenLetter = 0;												//index within the string
	int square = 0;														//index within the board
	while ((square < 64) && (FenLetter < fen[0].length()))
	{
		char letter = fen[0].at(FenLetter);
		//ugly code, but surely this is the only time in the code that we iterate over squares in a different order than the Square enum
		//the squares values go up as you move up the board towards black's starting side, but a fen starts from the black side and works downwards
		Square sq = static_cast<Square>((RANK_8 - square / 8) * 8 + square % 8);

		switch (letter)
		{
		case 'p': SetSquare(sq, BLACK_PAWN); break;
		case 'r': SetSquare(sq, BLACK_ROOK); break;
		case 'n': SetSquare(sq, BLACK_KNIGHT); break;
		case 'b': SetSquare(sq, BLACK_BISHOP); break;
		case 'q': SetSquare(sq, BLACK_QUEEN); break;
		case 'k': SetSquare(sq, BLACK_KING); break;
		case 'P': SetSquare(sq, WHITE_PAWN); break;
		case 'R': SetSquare(sq, WHITE_ROOK); break;
		case 'N': SetSquare(sq, WHITE_KNIGHT); break;
		case 'B': SetSquare(sq, WHITE_BISHOP); break;
		case 'Q': SetSquare(sq, WHITE_QUEEN); break;
		case 'K': SetSquare(sq, WHITE_KING); break;
		case '/': square--; break;
		case '1': break;
		case '2': square += 1; break;
		case '3': square += 2; break;
		case '4': square += 3; break;
		case '5': square += 4; break;
		case '6': square += 5; break;
		case '7': square += 6; break;
		case '8': square += 7; break;
		default: return false;					//bad fen
		}
		square++;
		FenLetter++;
	}

	RecalculateWhiteBlackBoards();
	return true;
}

void BitBoard::SaveBoard()
{
	previousBoards.emplace_back(previousBoards.back());
}

void BitBoard::RestorePreviousBoard()
{
	assert(previousBoards.size() > 1);
	previousBoards.pop_back();
	RecalculateWhiteBlackBoards();
}

void BitBoard::RecalculateWhiteBlackBoards()
{
	WhitePieces = GetPieceBB(WHITE_PAWN) | GetPieceBB(WHITE_KNIGHT) | GetPieceBB(WHITE_BISHOP) | GetPieceBB(WHITE_ROOK) | GetPieceBB(WHITE_QUEEN) | GetPieceBB(WHITE_KING);
	BlackPieces = GetPieceBB(BLACK_PAWN) | GetPieceBB(BLACK_KNIGHT) | GetPieceBB(BLACK_BISHOP) | GetPieceBB(BLACK_ROOK) | GetPieceBB(BLACK_QUEEN) | GetPieceBB(BLACK_KING);
}

uint64_t BitBoard::GetPiecesColour(Players colour) const
{
	if (colour == WHITE)
		return GetWhitePieces();
	else
		return GetBlackPieces();
}

void BitBoard::SetSquare(Square square, Pieces piece)
{
	assert(square < N_SQUARES);
	assert(piece < N_PIECES);

	ClearSquare(square);

	if (piece < N_PIECES)	//it is possible we might set a square to be empty using this function rather than using the ClearSquare function below. 
		previousBoards.back()[piece] |= SquareBB[square];

	RecalculateWhiteBlackBoards();
}

uint64_t BitBoard::GetPieceBB(Pieces piece) const
{
	return previousBoards.back()[piece];
}

void BitBoard::ClearSquare(Square square)
{
	assert(square < N_SQUARES);

	for (int i = 0; i < N_PIECES; i++)
	{
		previousBoards.back()[i] &= ~SquareBB[square];
	}

	RecalculateWhiteBlackBoards();
}

uint64_t BitBoard::GetPieceBB(PieceTypes pieceType, Players colour) const
{
	return GetPieceBB(Piece(pieceType, colour));
}

Square BitBoard::GetKing(Players colour) const
{
	assert(GetPieceBB(KING, colour) != 0);	//assert only runs in debug so I don't care about the double call
	return static_cast<Square>(LSB(GetPieceBB(KING, colour)));
}

bool BitBoard::IsEmpty(Square square) const
{
	assert(square != N_SQUARES);
	return ((GetAllPieces() & SquareBB[square]) == 0);
}

bool BitBoard::IsOccupied(Square square) const
{
	assert(square != N_SQUARES);
	return (!IsEmpty(square));
}

bool BitBoard::IsOccupied(Square square, Players colour) const
{
	assert(square != N_SQUARES);
	return colour == WHITE ? (GetWhitePieces() & SquareBB[square]) : (GetBlackPieces() & SquareBB[square]);
}

Pieces BitBoard::GetSquare(Square square) const
{
	for (int i = 0; i < N_PIECES; i++)
	{
		if ((GetPieceBB(static_cast<Pieces>(i)) & SquareBB[square]) != 0)
			return static_cast<Pieces>(i);
	}

	return N_PIECES;
}

uint64_t BitBoard::GetAllPieces() const
{
	return GetWhitePieces() | GetBlackPieces();
}

uint64_t BitBoard::GetEmptySquares() const
{
	return ~GetAllPieces();
}

uint64_t BitBoard::GetWhitePieces() const
{
	return WhitePieces;
}

uint64_t BitBoard::GetBlackPieces() const
{
	return BlackPieces;
}
