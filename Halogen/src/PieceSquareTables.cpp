#include "PieceSquareTables.h"
#include <iostream>

int PieceSquareTables[N_STAGES][N_PIECES][N_SQUARES];
void AddPieceSquareTableLayer(unsigned int stage, unsigned int piece, int layer[64]);			

int PawnSquareValuesMid[64] = {
  0,  0,  0,  0,  0,  0,  0,  0,
 16, 36, 17, 18, 23, 92, 91, 37,
 31, 44, 42, 32, 52, 70, 96, 50,
 31, 43, 45, 65, 66, 78, 69, 38,
 53, 64, 59, 76, 92, 78, 79, 46,
 35, 26, 34, 29, 43, 58, 35, 21,
 57, 59, 44, 49, 48, 49, 43, 47,
  0,  0,  0,  0,  0,  0,  0,  0,
};
int PawnSquareValuesEndGame[64] = { 
  0,  0,  0,  0,  0,  0,  0,  0,
 87, 85, 90, 59, 67, 62, 64, 60,
 80, 79, 70, 72, 72, 63, 59, 58,
 88, 87, 72, 60, 62, 59, 67, 70,
 95, 84, 70, 54, 56, 57, 77, 79,
101, 91, 60, 27, 18, 48, 73, 80,
 65, 63, 31, 27, 32, 30, 44, 57,
  0,  0,  0,  0,  0,  0,  0,  0,
};
int KnightSquareValues[64] = {
-52,-39,-35,-29,-26,-30,-37,-51,
-38,-30,-15,-17,-14, -1,-25,-34,
-33,-17, -7,  4,  5, -7,  0,-31,
-24, -7,  7, 12, 11, 11,  4,-24,
-22,  3, 21, 32, 25, 25,  2,-15,
-36, -6,  9, 19, 14, 15,  3,-32,
-52,-23,  4,  6, -7, -5,-22,-44,
-70,-44,-29,-37,-24,-40,-42,-59,
};
int BishopSquareValues[64] = {
-31,-12,-31,-13,-14,-29,-15,-21,
 -6, -3, -1, -8, -1,  1, 16,-15,
-16, -1,  6,  6,  6,  9,  3, -9,
-15,  1,  4, 18, 15,  8, -5,-11,
-10, -4,  9, 19, 19, 13, -3,-10,
-18, -2,  5,  4,  5,  6,  4,-10,
-20, -1, -5, -7, -1,  0, -4,-36,
-23,-15,-20,-16,-12,-14,-15,-22,
};
int RookSquareValues[64] = {
 -7, -5, -1,  3,  1, -2,-12,-14,
-22,-13,-14, -9,-10, -5,-12,-33,
-16, -9,-13,-15,-13, -8, -4,-18,
-10, -4, -6, -8, -9, -4, -4, -9,
 -5, -4,  7, -3,  1,  8, -2, -5,
  2,  6,  6,  1, -3,  1,  5, -5,
  0,  0,  5,  3, -7,  3,  1, -5,
 16,  8, 14, 13, 12,  9,  6, 11,
};
int QueenSquareValues[64] = {
 -8,-13,-10,  5,-13,-31,-19,-27,
-23, -7,  7,  3,  5, -2,-11, -7,
 -7,  1,  2, -1, -1,  2,  4, -5,
-11, -8, -1,  4,  9,  6,  1,  3,
-17,-14, -3,  3, 18,  8,  8, -3,
-18,-10,  1, 12, 21, 18, 12, 10,
-26,-21,  0,  5,  9, 13,  4,  0,
-28, -1,  1, -1,  7,  1, -5, -3,
};
int KingSquareMid[64] = {
 19, 77, 63,-17, 33, 14,101, 76,
 24, 38, 23, -2,  6, 35, 95, 86,
 -6,-13,-11, -9, -4,  8, 29, 17,
-20,-24,-28,-37,-33,-20,-19,-12,
-28,-33,-37,-49,-48,-34,-28,-26,
-25,-36,-37,-48,-49,-32,-29,-24,
-29,-35,-38,-47,-50,-33,-36,-28,
-30,-40,-39,-49,-50,-40,-40,-30,
};
int KingSquareEndGame[64] = {
-60,-29,-16,-11,-14,  2,-27,-39,
-29, -8, 19, 22, 31, 23,  2,-16,
-24,  8, 26, 35, 43, 37, 28,  4,
-29,  0, 33, 43, 47, 44, 30,-10,
-26, 17, 36, 43, 42, 43, 33, -2,
-20,  6, 24, 28, 24, 45, 27,-10,
-29,-12,  7,  7,  7, 16,-12,-20,
-53,-30,-28,-31,-29,-26,-27,-49,
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
