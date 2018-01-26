#include "BitBoard.h"

BitBoard::BitBoard()
{
	for (int i = 0; i < N_PIECES; i++)
	{
		m_Bitboard[i] = EMPTY;
		AttackTable[i] = EMPTY;
	}
}


BitBoard::~BitBoard()
{
}

unsigned int BitBoard::GetSquare(unsigned int square) const
{
	for (int i = 0; i < N_PIECES; i++)
	{
		if ((m_Bitboard[i] & SquareBB[square]) != 0)
			return i;
	}

	return N_PIECES;
}

void BitBoard::SetSquare(unsigned int square, unsigned int piece)
{
	ClearSquare(square);
	m_Bitboard[piece] |= SquareBB[square];
}

void BitBoard::ClearSquare(unsigned int square)
{
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

	uint64_t WhiteThreats = EMPTY;
	uint64_t BlackThreats = EMPTY;
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

	GenerateAttackTables();

	return true;
}

void BitBoard::GenerateAttackTables()
{
	WhiteThreats = EMPTY;
	BlackThreats = EMPTY;

	for (int i = 0; i < N_PIECES; i++)
	{
		AttackTable[i] = EMPTY;
	}

	uint64_t mask = GetAllPieces();

	uint64_t movePiece = GetPieceBB(WHITE_KNIGHT);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = KnightAttacks[start] & (~GetWhitePieces());
		AttackTable[WHITE_KNIGHT] |= moves;
	}

	movePiece = GetPieceBB(BLACK_KNIGHT);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = KnightAttacks[start] & (~GetBlackPieces());
		AttackTable[BLACK_KNIGHT] |= moves;
	}

	movePiece = GetPieceBB(WHITE_PAWN);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = WhitePawnAttacks[start] & (~GetWhitePieces());
		AttackTable[WHITE_PAWN] |= moves;
	}

	movePiece = GetPieceBB(BLACK_PAWN);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = BlackPawnAttacks[start] & (~GetBlackPieces());
		AttackTable[BLACK_PAWN] |= moves;
	}

	movePiece = GetPieceBB(WHITE_KING);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = KingAttacks[start] & (~GetWhitePieces());
		AttackTable[WHITE_KING] |= moves;
	}

	movePiece = GetPieceBB(BLACK_KING);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = KingAttacks[start] & (~GetBlackPieces());
		AttackTable[BLACK_KING] |= moves;
	}

	movePiece = GetPieceBB(WHITE_BISHOP);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = BishopAttacks[start] & (~GetWhitePieces());

		while (moves != 0)
		{
			unsigned int end = bitScanFowardErase(moves);
			if (mayMove(start, end, mask))
				AttackTable[WHITE_BISHOP] |= SquareBB[end];
		}
	}

	movePiece = GetPieceBB(BLACK_BISHOP);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = BishopAttacks[start] & (~GetBlackPieces());

		while (moves != 0)
		{
			unsigned int end = bitScanFowardErase(moves);
			if (mayMove(start, end, mask))
				AttackTable[BLACK_BISHOP] |= SquareBB[end];
		}
	}

	movePiece = GetPieceBB(WHITE_ROOK);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = RookAttacks[start] & (~GetWhitePieces());

		while (moves != 0)
		{
			unsigned int end = bitScanFowardErase(moves);
			if (mayMove(start, end, mask))
				AttackTable[WHITE_ROOK] |= SquareBB[end];
		}
	}

	movePiece = GetPieceBB(BLACK_ROOK);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = RookAttacks[start] & (~GetBlackPieces());

		while (moves != 0)
		{
			unsigned int end = bitScanFowardErase(moves);
			if (mayMove(start, end, mask))
				AttackTable[BLACK_ROOK] |= SquareBB[end];
		}
	}

	movePiece = GetPieceBB(WHITE_QUEEN);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = QueenAttacks[start] & (~GetWhitePieces());

		while (moves != 0)
		{
			unsigned int end = bitScanFowardErase(moves);
			if (mayMove(start, end, mask))
				AttackTable[WHITE_QUEEN] |= SquareBB[end];
		}
	}

	movePiece = GetPieceBB(BLACK_QUEEN);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = QueenAttacks[start] & (~GetBlackPieces());

		while (moves != 0)
		{
			unsigned int end = bitScanFowardErase(moves);
			if (mayMove(start, end, mask))
				AttackTable[BLACK_QUEEN] |= SquareBB[end];
		}
	}

	WhiteThreats |= AttackTable[WHITE_PAWN] | AttackTable[WHITE_KNIGHT] | AttackTable[WHITE_BISHOP] | AttackTable[WHITE_QUEEN] | AttackTable[WHITE_ROOK] | AttackTable[WHITE_KING];
	BlackThreats |= AttackTable[BLACK_PAWN] | AttackTable[BLACK_KNIGHT] | AttackTable[BLACK_BISHOP] | AttackTable[BLACK_QUEEN] | AttackTable[BLACK_ROOK] | AttackTable[BLACK_KING];
}

bool BitBoard::IsEmpty(unsigned int BitBoard) const
{
	uint64_t mask = SquareBB[BitBoard] & GetEmptySquares();
	if (mask != 0)
		return true;
	return false;
}

bool BitBoard::IsOccupied(unsigned int BitBoard) const
{
	return !IsEmpty(BitBoard);
}

bool BitBoard::IsOccupied(unsigned int BitBoard, bool colour) const
{
	if ((SquareBB[BitBoard] & GetPiecesColour(colour)) != 0)
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
	return m_Bitboard[piece];
}

uint64_t BitBoard::GetPieceBB(unsigned int pieceType, bool colour) const
{
	return m_Bitboard[pieceType + 6 * (colour)];
}

unsigned int BitBoard::GetKing(bool colour) const
{
	uint64_t squares = GetPieceBB(KING, colour);

	if (squares == 0)
		return N_SQUARES;
	return bitScanForward(squares);
}
