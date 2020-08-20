#include "PieceSquareTables.h"
#include <iostream>

int PieceSquareTables[N_STAGES][N_PIECES][N_SQUARES];
void AddPieceSquareTableLayer(unsigned int stage, unsigned int piece, int layer[64]);			

int PawnSquareValuesMid[64] = {
   0,   0,   0,   0,   0,   0,   0,   0,
 -14,   6, -13,  -4,   1,  78,  77,  15,
  -4,  13,   9,   5,  28,  53,  79,  26,
  -8,   5,  10,  29,  35,  59,  45,  10,
  12,  31,  22,  38,  48,  44,  49,  13,
 -28, -50, -42, -44, -34,  -4, -46, -41,
 -50, -45, -67, -60, -66, -63, -78, -76,
   0,   0,   0,   0,   0,   0,   0,   0,
};
int PawnSquareValuesEndGame[64] = { 
   0,   0,   0,   0,   0,   0,   0,   0,
  35,  30,  36,  -3,  10,   6,   5,   2,
  29,  26,  23,  22,  21,   4,   2,   3,
  40,  33,  17,   8,   4,   2,  12,  14,
  47,  35,  25,   4,   9,   5,  25,  26,
  43,  28,  -4, -44, -61, -19,   2,  20,
 -24, -33, -77, -84, -88, -92, -69, -50,
   0,   0,   0,   0,   0,   0,   0,   0,
};
int KnightSquareValues[64] = {
 -29, -16, -17,  -7,   3, -15, -13, -27,
 -12,  -8,  -7,   3,   0,  11,   2, -12,
 -18,  -4,  -1,   9,  13,   0,   9, -12,
  -2,   8,  12,  20,  21,  21,  24,  -2,
   1,  14,  21,  37,  33,  29,  13,  12,
 -16,  14,  19,  24,  18,  29,  23,  -7,
 -30,  -4,  18,  22,   7,   0,  -3, -29,
 -63, -25,  -1, -14,   0, -31, -19, -46,
};
int BishopSquareValues[64] = {
 -20,   2, -12,  -1,  -2, -14,  -6, -12,
   3,   7,   5,   0,   7,   8,  29,  -7,
  -6,   8,  10,   7,  11,  15,   6,   6,
   1,   2,   4,   9,  11,   3,   1,   2,
  -2,  -1,   4,  14,   9,  14,  -4,   0,
 -11,   2,   9,   0,   0,   7,  10,  -7,
 -15,   1,  -4,  -7,   3,  10,  -1, -38,
 -12, -10, -19,  -3,   0, -10,  -5, -12,
};
int RookSquareValues[64] = {
 -12,  -6,  -1,   1,   1,  -5, -13, -11,
 -26, -13, -15,  -7,  -9,  -2,  -8, -29,
 -15,  -9, -13, -15,  -9,  -7,  -1, -14,
  -7,   0,   2,  -5,  -8,  -1,   2,  -9,
   3,  -2,  10,  -1,   6,   8,   2,   2,
   8,  10,   7,   5,  -1,   7,  10,  -3,
  14,  19,  19,  16,   5,  16,  10,  12,
  14,   9,  10,  10,   7,  11,  10,  10,
};
int QueenSquareValues[64] = {
  -1,  -3,   2,  17,  -8, -27, -20, -30,
 -24,  -6,  10,   1,   9,   4, -12,  -9,
  -2,   1,  -1,  -2,  -6,   5,  10,  -1,
  -5, -11,   1,  -8,   1,   0,   1,   9,
 -15, -14,  -8,  -5,  17,   6,  19,   3,
 -18,  -9,  -1,  15,  29,  27,  25,  24,
 -28, -33,   0,   8,  13,  17,   3,   6,
 -22,   5,   3,   4,  21,   5,  -3,  11,
};
int KingSquareMid[64] = {
  22, 137, 113,  10,  70,  69, 156, 141,
  30,  61,  53,  10,  56,  90, 161, 153,
 -10,  -1,   6,  13,  29,  61,  90,  50,
 -28, -24, -24, -34, -21,   3,  -3,  -2,
 -37, -37, -39, -49, -43, -32, -22, -29,
 -35, -43, -42, -52, -54, -34, -31, -32,
 -44, -44, -48, -57, -60, -44, -46, -43,
 -47, -55, -54, -64, -65, -55, -53, -46,
};
int KingSquareEndGame[64] = {
 -73, -35, -18,   6,  -4,   1, -30, -51,
 -33,   0,  20,  31,  33,  20,  -4, -26,
 -29,  15,  34,  44,  46,  36,  25,   9,
 -39,   4,  42,  51,  56,  51,  38,  -4,
 -34,  23,  41,  47,  48,  54,  46,   7,
 -32,   6,  25,  24,  18,  52,  36,  -4,
 -52, -18,  -2,  -5,   0,  13, -15, -35,
 -83, -57, -53, -54, -49, -44, -45, -73,
};

void InitializePieceSquareTable()
{
	for (int i = 0; i < N_STAGES; i++)
	{
		AddPieceSquareTableLayer(i, WHITE_KNIGHT, KnightSquareValues);
		AddPieceSquareTableLayer(i, BLACK_KNIGHT, KnightSquareValues);
		AddPieceSquareTableLayer(i, WHITE_BISHOP, BishopSquareValues);
		AddPieceSquareTableLayer(i, BLACK_BISHOP, BishopSquareValues);
		AddPieceSquareTableLayer(i, WHITE_QUEEN, QueenSquareValues);
		AddPieceSquareTableLayer(i, BLACK_QUEEN, QueenSquareValues);
		AddPieceSquareTableLayer(i, WHITE_ROOK, RookSquareValues);
		AddPieceSquareTableLayer(i, BLACK_ROOK, RookSquareValues);
	}

	AddPieceSquareTableLayer(MIDGAME, WHITE_KING, KingSquareMid);
	AddPieceSquareTableLayer(MIDGAME, BLACK_KING, KingSquareMid);
	AddPieceSquareTableLayer(ENDGAME, WHITE_KING, KingSquareEndGame);
	AddPieceSquareTableLayer(ENDGAME, BLACK_KING, KingSquareEndGame);

	AddPieceSquareTableLayer(MIDGAME, WHITE_PAWN, PawnSquareValuesMid);
	AddPieceSquareTableLayer(MIDGAME, BLACK_PAWN, PawnSquareValuesMid);
	AddPieceSquareTableLayer(ENDGAME, WHITE_PAWN, PawnSquareValuesEndGame);
	AddPieceSquareTableLayer(ENDGAME, BLACK_PAWN, PawnSquareValuesEndGame);
}

void AddPieceSquareTableLayer(unsigned int stage, unsigned int piece, int layer[64])
{
	if (ColourOfPiece(piece) == WHITE)
	{
		for (int i = 0; i < N_SQUARES; i++)
		{
			PieceSquareTables[stage][piece][i] = layer[i];
		}
	}

	if (ColourOfPiece(piece) == BLACK)
	{
		for (int i = 0; i < N_SQUARES; i++)
		{
			PieceSquareTables[stage][piece][i] = layer[GetPosition(GetFile(i), RANK_8 - GetRank(i))];
		}
	}
}
