#include "BitBoard.h"

BitBoard::BitBoard()
{
}


BitBoard::~BitBoard()
{
}

unsigned int BitBoard::GetSquare(unsigned int square)
{
	if (square < 0 || square >= N_SQUARES)
		return N_PIECES;

	for (int i = 0; i < N_PIECES; i++)
	{
		if ((m_Bitboard[i] & SquareBB[square]) != 0)
			return i;
	}

	return N_PIECES;
}

void BitBoard::SetSquare(unsigned int square, unsigned int piece)
{
	if (square < 0 || square >= N_SQUARES || piece < 0 || piece >= N_PIECES)
		return;

	ClearSquare(square);
	m_Bitboard[piece] |= SquareBB[square];
}

void BitBoard::ClearSquare(unsigned int square)
{
	if (square < 0 || square >= N_SQUARES)
		return;

	for (int i = 0; i < N_PIECES; i++)
	{
		m_Bitboard[i] &= ~SquareBB[square];
	}
}

void BitBoard::Reset()
{
	for (int i = 0; i < N_PIECES; i++)
	{
		m_Bitboard[i] &= ~m_Bitboard[i];
	}
}

bool BitBoard::Empty(unsigned int position)
{
	if (position < 0 || position >= N_SQUARES)
		return false;

	uint64_t mask = SquareBB[position] & EmptySquares();
	if (mask != 0)
		return true;
	return false;
}

bool BitBoard::Occupied(unsigned int position)
{
	return !Empty(position);
}

bool BitBoard::Occupied(unsigned int position, unsigned int colour)
{
	if (position < 0 || position >= N_SQUARES)
		return true;

	uint64_t mask;

	if (colour == WHITE)
		mask = SquareBB[position] & WhitePieces();
	if (colour == BLACK)
		mask = SquareBB[position] & BlackPieces();

	if (mask != 0)
		return true;
	return false;
}

uint64_t BitBoard::AllPieces()
{
	return WhitePieces() | BlackPieces();
}

uint64_t BitBoard::EmptySquares()
{
	return ~AllPieces();
}

uint64_t BitBoard::WhitePieces()
{
	return m_Bitboard[WHITE_PAWN] | m_Bitboard[WHITE_KNIGHT] | m_Bitboard[WHITE_BISHOP] | m_Bitboard[WHITE_ROOK] | m_Bitboard[WHITE_QUEEN] | m_Bitboard[WHITE_KING];
}

uint64_t BitBoard::BlackPieces()
{
	return m_Bitboard[BLACK_PAWN] | m_Bitboard[BLACK_KNIGHT] | m_Bitboard[BLACK_BISHOP] | m_Bitboard[BLACK_ROOK] | m_Bitboard[BLACK_QUEEN] | m_Bitboard[BLACK_KING];
}

uint64_t BitBoard::ColourPieces(bool colour)
{
	if (colour == WHITE)
		return WhitePieces();
	else
		return BlackPieces();
}

uint64_t BitBoard::GetPieces(unsigned int piece)
{
	if (piece < 0 || piece >= N_PIECES)
		return EMPTY;
	return m_Bitboard[piece];
}

bool BitBoard::ColourOfPiece(unsigned int piece)
{
	if (piece <= WHITE_KING)
		return WHITE;
	return BLACK;
}
