#include "BitBoard.h"
#include <stdexcept>

BitBoard::BitBoard()
{

}

BitBoard::~BitBoard()
{
}

void BitBoard::SetSquare(unsigned int square, unsigned int piece)
{
	assert(square < N_SQUARES);
	assert(piece < N_PIECES);

	ClearSquare(square);

	if (piece < N_PIECES)	//it is possible we might set a square to be empty using this function rather than using the ClearSquare function below. 
		m_Bitboard[piece] |= SquareBB[square];
}

void BitBoard::ClearSquare(unsigned int square)
{
	assert(square < N_SQUARES);

	for (int i = 0; i < N_PIECES; i++)
	{
		m_Bitboard[i] &= ~SquareBB[square];
	}
}

void BitBoard::ResetBoard()
{
	previousBoards.clear();

	for (int i = 0; i < N_PIECES; i++)
	{
		m_Bitboard[i] = EMPTY;
	}
}

bool BitBoard::InitialiseBoardFromFen(std::vector<std::string> fen)
{
	ResetBoard();

	int FenLetter = 0;													//index within the string
	int square = 0;														//index within the board
	while ((square < 64) && (FenLetter < fen[0].length()))
	{
		char letter = fen[0].at(FenLetter);
		int sq = GetPosition(GetFile(square), 7 - GetRank(square));		//the squares of the board go up as you move up the board towards blacks starting side, but a fen starts from the black side and works downwards

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

	return true;
}

void BitBoard::SaveBoard()
{
	previousBoards.push_back(static_cast<BitBoardData>(*this));
}

void BitBoard::RestorePreviousBoard()
{
	assert(previousBoards.size() != 0);


	for (int i = 0; i < N_PIECES; i++)
	{
		m_Bitboard[i] = previousBoards.back().GetPieceBB(i);
	}

	previousBoards.pop_back();
}

BitBoardData BitBoard::GetPreviousBoard()
{
	assert(previousBoards.size() != 0);
	return previousBoards.back();
}

bool BitBoard::IsEmpty(unsigned int square) const
{
	assert(square < N_SQUARES);

	return ((SquareBB[square] & GetEmptySquares()) != 0);
}

bool BitBoard::IsOccupied(unsigned int square) const
{
	assert(square < N_SQUARES);

	return !IsEmpty(square);
}

bool BitBoard::IsOccupied(unsigned int square, bool colour) const
{
	assert(square < N_SQUARES);

	return ((SquareBB[square] & GetPiecesColour(colour)) != 0);
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
	return m_Bitboard[WHITE_PAWN] | m_Bitboard[WHITE_KNIGHT] | m_Bitboard[WHITE_BISHOP] | m_Bitboard[WHITE_ROOK] | m_Bitboard[WHITE_QUEEN] | m_Bitboard[WHITE_KING];
}

uint64_t BitBoard::GetBlackPieces() const
{
	return m_Bitboard[BLACK_PAWN] | m_Bitboard[BLACK_KNIGHT] | m_Bitboard[BLACK_BISHOP] | m_Bitboard[BLACK_ROOK] | m_Bitboard[BLACK_QUEEN] | m_Bitboard[BLACK_KING];
}

uint64_t BitBoard::GetPiecesColour(bool colour) const
{
	if (colour == WHITE)
		return GetWhitePieces();
	else
		return GetBlackPieces();
}

uint64_t BitBoard::GetPieceBB(unsigned int pieceType, bool colour) const
{
	assert(pieceType < N_PIECE_TYPES);

	return m_Bitboard[pieceType + 6 * (colour)];
}

unsigned int BitBoard::GetKing(bool colour) const
{
	assert(GetPieceBB(KING, colour) != 0);	//assert only runs in debug so I don't care about the double call

	unsigned long index;
	_BitScanForward64(&index, GetPieceBB(KING, colour));
	return index;
}

BitBoardData::BitBoardData() : m_Bitboard {0}
{

}

uint64_t BitBoardData::GetPieceBB(unsigned int piece) const
{
	assert(piece < N_PIECES);

	return m_Bitboard[piece];
}

unsigned int BitBoardData::GetSquare(unsigned int square) const
{
	assert(square < N_SQUARES);

	for (int i = 0; i < N_PIECES; i++)
	{
		if ((m_Bitboard[i] & SquareBB[square]) != 0)
			return i;
	}

	return N_PIECES;
}
