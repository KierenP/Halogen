#include "BitBoard.h"
#include <stdexcept>

std::vector<BitBoard> previousBoards;

BitBoard::BitBoard() : AttackTable{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, m_Bitboard{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
{

}


BitBoard::~BitBoard()
{
}

unsigned int BitBoard::GetSquare(unsigned int square) const
{
	assert(square < N_SQUARES);

	for (int i = 0; i < N_PIECES; i++)
	{
		if ((m_Bitboard[i] & SquareBB[square]) != 0)
			return i;
	}

	return N_PIECES;
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

void BitBoard::Reset()
{
	for (int i = 0; i < N_PIECES; i++)
	{
		m_Bitboard[i] = EMPTY;
		AttackTable[i] = EMPTY;
	}
}

bool BitBoard::InitialiseBoardFromFen(std::vector<std::string> fen)
{
	Reset();

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
	previousBoards.push_back(*this);
}

void BitBoard::RestorePreviousBoard()
{
	assert(previousBoards.size() != 0);

	*this = previousBoards[previousBoards.size() - 1];
	previousBoards.erase(previousBoards.end() - 1);			//erase the last element
}

bool BitBoard::IsEmpty(unsigned int square) const
{
	assert(square < N_SQUARES);

	uint64_t mask = SquareBB[square] & GetEmptySquares();
	if (mask != 0)
		return true;
	return false;
}

bool BitBoard::IsOccupied(unsigned int square) const
{
	assert(square < N_SQUARES);

	return !IsEmpty(square);
}

bool BitBoard::IsOccupied(unsigned int square, bool colour) const
{
	assert(square < N_SQUARES);

	if ((SquareBB[square] & GetPiecesColour(colour)) != 0)
		return true;
	return false;
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

uint64_t BitBoard::GetPieceBB(unsigned int piece) const
{
	assert(piece < N_PIECES);

	return m_Bitboard[piece];
}

uint64_t BitBoard::GetPieceBB(unsigned int pieceType, bool colour) const
{
	assert(pieceType < N_PIECE_TYPES);

	return m_Bitboard[pieceType + 6 * (colour)];
}

unsigned int BitBoard::GetKing(bool colour) const
{
	uint64_t squares = GetPieceBB(KING, colour);

	assert(squares != 0);

	return bitScanForward(squares);
}
