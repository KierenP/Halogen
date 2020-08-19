#include "PieceSquareTables.h"
#include <iostream>

int PieceSquareTables[N_STAGES][N_PIECES][N_SQUARES];
void AddPieceSquareTableLayer(unsigned int stage, unsigned int piece, int layer[64]);			

int PawnSquareValuesMid[64] = {
   0,   0,   0,   0,   0,   0,   0,   0,
 -13,   8, -19, -17,  -7,  62,  70,   4,
  -5,  11,  -1,  -2,  15,  37,  66,  11,
 -11,  -3,  -3,  17,  19,  37,  31,  -5,
   1,  22,   7,  27,  31,  27,  36,  -2,
 -22, -34, -31, -28, -16,  -2, -30, -40,
 -19, -16, -34, -27, -32, -32, -43, -39,
   0,   0,   0,   0,   0,   0,   0,   0,
};
int PawnSquareValuesEndGame[64] = { 
   0,   0,   0,   0,   0,   0,   0,   0,
  28,  21,  31,  -1,   7,  -6,   0,  -6,
  21,  19,  11,  14,   6,  -7,  -7,  -7,
  30,  25,   8,  -4,  -9, -14,   1,   0,
  42,  29,  17,   2,  -7,  -5,  12,  16,
  50,  35,   3, -29, -46, -19,   9,  23,
  -4, -10, -46, -53, -54, -60, -43, -18,
   0,   0,   0,   0,   0,   0,   0,   0,
};
int KnightSquareValues[64] = {
 -32, -15, -17,  -7,  -2, -14, -13, -30,
 -16, -13,  -3,   3,   1,  12,  -3, -16,
 -16,  -4,   0,   9,  16,   1,  14, -14,
  -3,   8,  17,  22,  19,  22,  20,  -3,
   2,  23,  26,  37,  34,  33,  14,   5,
 -18,  13,  23,  25,  26,  31,  24, -12,
 -33,  -6,  19,  21,   7,   5,  -3, -29,
 -59, -28,  -8, -15,  -5, -28, -22, -45,
};
int BishopSquareValues[64] = {
 -21,  -2, -12,  -1,  -3, -13,  -7, -15,
   0,   5,   6,  -1,   9,   6,  29,  -9,
  -6,   7,  15,   9,  11,  16,   8,   1,
  -1,   5,   3,  14,  10,   3,  -1,   2,
  -2,   1,   8,  13,  14,  19,   0,  -7,
 -13,   3,  11,   8,   4,   7,   8,  -9,
 -16,   4,  -2,  -7,   4,   8,  -2, -36,
 -15, -10, -17,  -7,  -5,  -8,  -8, -15,
};
int RookSquareValues[64] = {
 -11,  -7,   1,   0,   0,  -4, -12, -13,
 -23, -11, -11,  -5, -10,  -4,  -9, -33,
 -16, -10, -11, -10, -12,  -3,  -4, -21,
  -8,  -1,   0,  -3,  -7,  -2,   1,  -9,
   3,  -2,  10,   1,   2,  11,   1,   0,
   7,   9,   5,   6,  -3,   5,  10,  -1,
  18,  16,  18,  18,   7,  13,   9,  12,
  17,  10,  12,  10,  12,  13,  10,   8,
};
int QueenSquareValues[64] = {
  -1,  -7,  -2,  18, -13, -29, -20, -28,
 -19,  -3,   9,   1,   9,   1, -11,  -8,
  -3,   2,   1,  -4,  -3,   3,   8,  -2,
  -6,  -8,  -2,  -2,   0,   3,   1,   6,
 -15, -15,  -3,   0,  15,  11,  20,   4,
 -15,  -6,   1,  13,  26,  25,  20,  20,
 -27, -29,   4,   8,  13,  19,   6,   6,
 -24,   2,   4,   3,  16,   3,  -2,   5,
};
int KingSquareMid[64] = {
  26, 114,  93,  -3,  61,  48, 135, 113,
  30,  55,  47,   5,  33,  71, 138, 126,
  -3,  -4,   2,   6,  18,  38,  69,  37,
 -18, -19, -20, -30, -20,  -4,  -4,  -3,
 -26, -30, -31, -44, -41, -29, -20, -21,
 -25, -34, -34, -46, -46, -27, -25, -22,
 -31, -34, -38, -48, -50, -33, -35, -30,
 -33, -43, -41, -51, -52, -42, -41, -33,
};
int KingSquareEndGame[64] = {
 -71, -36, -19,  -6, -11,  -1, -33, -48,
 -33,  -2,  18,  28,  32,  17,  -7, -25,
 -30,   8,  29,  39,  45,  37,  25,   5,
 -35,   0,  35,  49,  51,  46,  38,  -5,
 -32,  17,  41,  41,  43,  51,  38,   4,
 -27,   4,  25,  23,  21,  50,  32,  -7,
 -41, -15,   1,  -2,   1,  13, -15, -29,
 -70, -45, -42, -43, -41, -37, -36, -63,
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
