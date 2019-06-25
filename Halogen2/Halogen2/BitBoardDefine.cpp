#include "BitBoardDefine.h"
#include <stdexcept>

uint64_t EMPTY;
uint64_t UNIVERCE;

uint64_t RankBB[N_RANKS];
uint64_t FileBB[N_FILES];
uint64_t SquareBB[N_SQUARES];
uint64_t DiagonalBB[N_DIAGONALS];
uint64_t AntiDiagonalBB[N_ANTI_DIAGONALS];

uint64_t betweenArray[64][64];
uint64_t PassedPawnMaskWhite[N_SQUARES];
uint64_t PassedPawnMaskBlack[N_SQUARES];

uint64_t RookAttacks[N_SQUARES];
uint64_t KnightAttacks[N_SQUARES];
uint64_t BishopAttacks[N_SQUARES];
uint64_t QueenAttacks[N_SQUARES];
uint64_t KingAttacks[N_SQUARES];
uint64_t WhitePawnAttacks[N_SQUARES];
uint64_t BlackPawnAttacks[N_SQUARES];

uint64_t WhiteKingAttackSquares[N_SQUARES];
uint64_t BlackKingAttackSquares[N_SQUARES];

const int index64[64] = {
	0, 47,  1, 56, 48, 27,  2, 60,
	57, 49, 41, 37, 28, 16,  3, 61,
	54, 58, 35, 52, 50, 42, 21, 44,
	38, 32, 29, 23, 17, 11,  4, 62,
	46, 55, 26, 59, 40, 36, 15, 53,
	34, 51, 20, 43, 31, 22, 10, 45,
	25, 39, 14, 33, 19, 30,  9, 24,
	13, 18,  8, 12,  7,  6,  5, 63
};

const uint64_t debruijn64 = uint64_t(0x03f79d71b4cb0a89);

void BBInit()
{
	UNIVERCE = 0xffffffffffffffff;
	EMPTY = 0;

	//A loop didnt work, and if these have a problem debugging is hard so they were hard coded
	RankBB[RANK_1] = 0xff;
	RankBB[RANK_2] = 0xff00;
	RankBB[RANK_3] = 0xff0000;
	RankBB[RANK_4] = 0xff000000;
	RankBB[RANK_5] = 0xff00000000;
	RankBB[RANK_6] = 0xff0000000000;
	RankBB[RANK_7] = 0xff000000000000;
	RankBB[RANK_8] = 0xff00000000000000;

	FileBB[FILE_H] = 0x8080808080808080;
	FileBB[FILE_G] = 0x4040404040404040;
	FileBB[FILE_F] = 0x2020202020202020;
	FileBB[FILE_E] = 0x1010101010101010;
	FileBB[FILE_D] = 0x808080808080808;
	FileBB[FILE_C] = 0x404040404040404;
	FileBB[FILE_B] = 0x202020202020202;
	FileBB[FILE_A] = 0x101010101010101;

	DiagonalBB[DIAG_A1H8] = 0x8040201008040201;
	DiagonalBB[DIAG_A2G8] = 0x4020100804020100;
	DiagonalBB[DIAG_A3F8] = 0x2010080402010000;
	DiagonalBB[DIAG_A4E8] = 0x1008040201000000;
	DiagonalBB[DIAG_A5D8] = 0x804020100000000;
	DiagonalBB[DIAG_A6C8] = 0x402010000000000;
	DiagonalBB[DIAG_A7B8] = 0x201000000000000;
	DiagonalBB[DIAG_A8A8] = 0x100000000000000;
	DiagonalBB[DIAG_B1H7] = 0x80402010080402;
	DiagonalBB[DIAG_C1H6] = 0x804020100804;
	DiagonalBB[DIAG_D1H5] = 0x8040201008;
	DiagonalBB[DIAG_E1H4] = 0x80402010;
	DiagonalBB[DIAG_F1H3] = 0x804020;
	DiagonalBB[DIAG_G1H2] = 0x8040;
	DiagonalBB[DIAG_H1H1] = 0x80;

	AntiDiagonalBB[DIAG_H8H8] = 0x8000000000000000;
	AntiDiagonalBB[DIAG_G8H7] = 0x4080000000000000;
	AntiDiagonalBB[DIAG_F8H6] = 0x2040800000000000;
	AntiDiagonalBB[DIAG_E8H5] = 0x1020408000000000;
	AntiDiagonalBB[DIAG_D8H4] = 0x810204080000000;
	AntiDiagonalBB[DIAG_C8H3] = 0x408102040800000;
	AntiDiagonalBB[DIAG_B8H2] = 0x204081020408000;
	AntiDiagonalBB[DIAG_A8H1] = 0x102040810204080;
	AntiDiagonalBB[DIAG_A7G1] = 0x1020408102040;
	AntiDiagonalBB[DIAG_A6F1] = 0x10204081020;
	AntiDiagonalBB[DIAG_A5E1] = 0x102040810;
	AntiDiagonalBB[DIAG_A4D1] = 0x1020408;
	AntiDiagonalBB[DIAG_A3C1] = 0x10204;
	AntiDiagonalBB[DIAG_A2B1] = 0x102;
	AntiDiagonalBB[DIAG_A1A1] = 0x1;


	for (int i = SQ_A1; i < N_SQUARES; i++)
	{
		SquareBB[i] = 1i64 << i;
	}

	for (int i = 0; i < 64; i++)
	{
		for (int j = 0; j < 64; j++)
		{
			betweenArray[i][j] = inBetween(i, j);
		}
	}

	for (int i = 0; i < 64; i++)
	{
		PassedPawnMaskWhite[i] = EMPTY;
		PassedPawnMaskBlack[i] = EMPTY;
		KingAttacks[i] = EMPTY;
		WhitePawnAttacks[i] = EMPTY;
		BlackPawnAttacks[i] = EMPTY;
		WhiteKingAttackSquares[i] = EMPTY;
		BlackKingAttackSquares[i] = EMPTY;

		for (int j = 0; j < 64; j++)
		{
			if (AbsFileDiff(i, j) <= 1)													//adjacent files or same file
			{
				if (GetRank(j) > GetRank(i))											//Ahead of i
					PassedPawnMaskWhite[i] ^= SquareBB[j];
				if (GetRank(j) < GetRank(i))											//behind of i
					PassedPawnMaskBlack[i] ^= SquareBB[j];
			}

			if (AbsFileDiff(i, j) <= 1 && AbsRankDiff(i, j) <= 1)
			{
				if (i != j)
				{
					KingAttacks[i] ^= SquareBB[j];
				}
			}

			if ((AbsFileDiff(i, j) == 1) && RankDiff(j, i) == 1)		//either side one ahead
			{
				WhitePawnAttacks[i] ^= SquareBB[j];
			}
			if ((AbsFileDiff(i, j) == 1) && RankDiff(j, i) == -1)		//either side one behind
			{
				BlackPawnAttacks[i] ^= SquareBB[j];
			}

			if ((AbsFileDiff(i, j) <= 1) && (RankDiff(j, i) > 1) && (RankDiff(j, i) <= 4))		//left, same file, or right
			{
				WhiteKingAttackSquares[i] ^= SquareBB[j];
			}
			if ((AbsFileDiff(i, j) <= 1) && (RankDiff(j, i) < -1) && (RankDiff(j, i) >= -4))		//left, same file, or right
			{
				BlackKingAttackSquares[i] ^= SquareBB[j];
			}

			if ((AbsRankDiff(i, j) == 1 && AbsFileDiff(i, j) == 2) || (AbsRankDiff(i, j) == 2 && AbsFileDiff(i, j) == 1))
				KnightAttacks[i] |= SquareBB[j];
		}

		WhiteKingAttackSquares[i] |= KingAttacks[i];
		BlackKingAttackSquares[i] |= KingAttacks[i];

		RookAttacks[i] = (RankBB[GetRank(i)] | FileBB[GetFile(i)]) ^ SquareBB[i];		//All the spaces in the same rank and file but not the starting square
		BishopAttacks[i] = (DiagonalBB[GetDiagonal(i)] | AntiDiagonalBB[GetAntiDiagonal(i)]) ^ SquareBB[i];
		QueenAttacks[i] = RookAttacks[i] | BishopAttacks[i];
 	}
}

char PieceToChar(unsigned int piece)
{
	if (piece > N_PIECES) throw std::invalid_argument("Bad paramiter to SetSquare()");

	char PieceChar[13] = { 'p', 'n', 'b', 'r', 'q', 'k', 'P', 'N', 'B', 'R', 'Q', 'K', ' ' };
	return PieceChar[piece];
}

unsigned int Piece(unsigned int piecetype, unsigned int colour)
{
	return piecetype + N_PIECE_TYPES * colour;
}

unsigned int GetFile(unsigned int square)
{
	return square % 8;
}

unsigned int GetRank(unsigned int square)
{
	return square / 8;
}

unsigned int GetPosition(unsigned int file, unsigned int rank)
{
	return rank * 8 + file;
}

unsigned int AbsRankDiff(unsigned int sq1, unsigned int sq2)
{
	return abs(static_cast<int>(GetRank(sq1)) - static_cast<int>(GetRank(sq2)));
}

unsigned int AbsFileDiff(unsigned int sq1, unsigned int sq2)
{
	return abs(static_cast<int>(GetFile(sq1)) - static_cast<int>(GetFile(sq2)));
}

int RankDiff(unsigned int sq1, unsigned int sq2)
{
	return static_cast<int>(GetRank(sq1)) - static_cast<int>(GetRank(sq2));
}

int FileDiff(unsigned int sq1, unsigned int sq2)
{
	return static_cast<int>(GetFile(sq1)) - static_cast<int>(GetFile(sq2));
}

unsigned int GetDiagonal(unsigned int square)
{
	return (RANK_8 - GetRank(square)) + GetFile(square);
}

unsigned int GetAntiDiagonal(unsigned int square)
{
	return RANK_8 + FILE_H - GetRank(square) - GetFile(square);
}

unsigned int GetBitCount(uint64_t bb)
{
	unsigned int num = 0;

	while (bb != 0)
	{
		bitScanFowardErase(bb);
		num++;
	}

	return num;
}

unsigned int AlgebraicToPos(std::string str)
{
	if (str == "-")
		return -1;

	if (str.length() < 2) throw std::invalid_argument("Bad paramiter to SetSquare()");
	return (str[0] - 97) + (str[1] - 49) * 8;		
}

unsigned int ColourOfPiece(unsigned int piece)
{
	if (piece <= BLACK_KING)
		return BLACK;
	return WHITE;
}

int bitScanForward(uint64_t bb)
{
	if (bb == EMPTY) throw std::invalid_argument("Bad paramiter to SetSquare()");

	return index64[((bb ^ (bb - 1)) * debruijn64) >> 58];
}

int bitScanFowardErase(uint64_t &bb)
{
	int index = bitScanForward(bb);
	bb ^= SquareBB[index];
	return index;
}

//Not my code
uint64_t inBetween(unsigned int sq1, unsigned int sq2)
{
	const int64_t a2a7 = uint64_t(0x0001010101010100);
	const int64_t b2g7 = uint64_t(0x0040201008040200);
	const int64_t h1b7 = uint64_t(0x0002040810204080); /* Thanks Dustin, g2b7 did not work for c1-a3 */
	int64_t btwn, line, rank, file;

	btwn = (UNIVERCE << sq1) ^ (UNIVERCE << sq2);
	file = (sq2 & 7) - (sq1 & 7);
	rank = ((sq2 | 7) - sq1) >> 3;
	line = ((file & 7) - 1) & a2a7;					/* a2a7 if same file */
	line += 2 * (((rank & 7) - 1) >> 58);			/* b1g1 if same rank */
	line += (((rank - file) & 15) - 1) & b2g7;		/* b2g7 if same diagonal */
	line += (((rank + file) & 15) - 1) & h1b7;		/* h1b7 if same antidiag */
	line *= btwn & -btwn;							/* mul acts like shift by smaller square */
	return line & btwn;								/* return the bits on that line in-between */
}

uint64_t inBetweenCache(unsigned int from, unsigned int to)
{
	if (from >= N_SQUARES) throw std::invalid_argument("Bad paramiter to SetSquare()");
	if (to >= N_SQUARES) throw std::invalid_argument("Bad paramiter to SetSquare()");

	return betweenArray[from][to];
}

bool mayMove(unsigned int from, unsigned int to, uint64_t pieces)
{
	return (inBetweenCache(from, to) & pieces) == 0;
}
