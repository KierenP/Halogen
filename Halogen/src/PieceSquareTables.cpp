#include "PieceSquareTables.h"
#include <iostream>

int PieceSquareTables[N_STAGES][N_PIECES][N_SQUARES];
void AddPieceSquareTableLayer(unsigned int stage, unsigned int piece, int layer[64]);			

int PawnSquareValuesMid[64] = {
   0,   0,   0,   0,   0,   0,   0,   0,
  90, 114,  78,  84,  95, 170, 189, 108,
  98, 113,  99, 103, 120, 146, 180, 117,
  84,  98,  98, 116, 121, 142, 140,  97,
 110, 123, 111, 129, 132, 123, 146, 105,
  28, -24,   0,  17, -42, -28,-126, -27,
-383,-377,-440,-376,-470,-505,-492,-529,
   0,   0,   0,   0,   0,   0,   0,   0,
};
int PawnSquareValuesEndGame[64] = { 
   0,   0,   0,   0,   0,   0,   0,   0,
  66,  56,  65,  20,  27,  18,  25,  12,
  52,  50,  43,  48,  30,  13,  17,  10,
  68,  54,  32,  24,   8,   4,  20,  19,
  77,  56,  44,  29,  13,  12,  31,  37,
 112, 115,  75,  38,  16,  45,  87,  86,
-124,-149,-196,-231,-268,-266,-245,-169,
   0,   0,   0,   0,   0,   0,   0,   0,
};
int KnightSquareValues[64] = {
 -13, -18, -17,  -5, -10, -19, -13,  -7,
 -10, -19, -12,   2,   2,  11,   4, -18,
 -21,  -7,   1,  13,  14,   5,  13, -21,
  -7,   7,  16,  25,  24,  21,  26,  -5,
   3,  18,  28,  48,  36,  41,  11,  16,
 -26,  15,  18,  29,  24,  39,  24, -10,
 -42, -14,  20,  24,   8,  -7,  -9, -43,
 -91, -11,   0, -16,  11, -44,  -7, -56,
};
int BishopSquareValues[64] = {
 -18,   4, -10,  -4,  -7, -13,  -2,  -8,
  13,   6,   8,  -3,   8,   9,  34,  -4,
  -6,   4,  12,   9,  13,  16,   6,   8,
   1,   3,   1,  13,  10,  -1,   1,   2,
  -2,  -2,   6,  12,   8,   8,   0,  -2,
 -16,   1,   5,  -3,  -2,   4,  13, -10,
 -17,  -6,  -6, -17,   8,  11, -11, -50,
 -12, -13, -18,   1,   2,  -3,   6,   0,
};
int RookSquareValues[64] = {
 -16,  -8,  -2,   1,   0,  -8, -18, -12,
 -28, -16, -14,  -7,  -9,  -3,  -6, -35,
 -21, -11, -15,  -9,  -6,  -7,  -4, -25,
  -6,  -1,   1,  -6, -10,  -8,   3, -14,
   3,   3,  12,  -3,   4,   3,   3,   4,
  11,  15,   4,   3, -11,   1,   1,   3,
  22,  20,  27,  30,  16,  21,  18,  25,
  17,  12,  10,   7,   7,  15,   8,   8,
};
int QueenSquareValues[64] = {
  -6,  -2,   8,  23, -10, -32, -40, -44,
 -15,  -4,  13,   7,  10,   2, -15,  -9,
   1,  -3,  -1,   3,  -5,   6,  18,  14,
  -7, -16,   0,   1,   4,   4,   4,   6,
 -15, -18,  -8, -11,  19,   7,  16,   8,
 -22, -23, -10,  13,  37,  38,  26,  32,
 -44, -50,  -3,  13,   9,  18, -21,  13,
 -28,   6,  12,  10,  32,   9,   8,  12,
};
int KingSquareMid[64] = {
  38, 396, 368, 202, 315, 307, 403, 393,
  38, 225, 254, 219, 288, 327, 416, 414,
 -92,  40, 121, 155, 213, 305, 383, 241,
-161, -85, -45, -45,  36,  99,  67, -10,
-196,-140,-125,-119, -91, -65, -66,-122,
-196,-165,-146,-154,-185,-136,-133,-154,
-222,-196,-204,-197,-210,-191,-208,-222,
-257,-252,-239,-251,-251,-245,-235,-252,
};
int KingSquareEndGame[64] = {
 -65, -76, -35,  -2, -21, -15, -52, -88,
  -7,   3,  15,  24,  21,   7, -32, -60,
  -1,  37,  50,  54,  53,  24,  -5,   2,
 -17,  41,  77,  81,  83,  72,  56,  32,
 -15,  57,  74,  86,  87,  94,  84,  55,
 -28,  51,  65,  65,  57,  98,  91,  38,
-128,  -2,  19,   7,  21,  51,  31, -51,
-241,-171,-144,-127, -96, -89,-109,-185,
};

void InitializePieceSquareTable()
{
	for (int i = 0; i < N_FILES; i++)
	{	//Important that texel tuning doesn't change these values.
		PawnSquareValuesMid[GetPosition(i, RANK_1)] = 0;
		PawnSquareValuesMid[GetPosition(i, RANK_8)] = 0;
		PawnSquareValuesEndGame[GetPosition(i, RANK_1)] = 0;
		PawnSquareValuesEndGame[GetPosition(i, RANK_8)] = 0;
	}

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
