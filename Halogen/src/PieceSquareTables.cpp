#include "PieceSquareTables.h"
#include <iostream>

int PieceSquareTables[N_STAGES][N_PIECES][N_SQUARES];
void AddPieceSquareTableLayer(unsigned int stage, unsigned int piece, int layer[64]);			

int PawnSquareValuesMid[64] = {
  0,  0,  0,  0,  0,  0,  0,  0,
 35, 52, 32, 36, 39,112,107, 58,
 56, 59, 58, 50, 71, 91,113, 67,
 47, 55, 59, 76, 81, 95, 90, 53,
 67, 78, 73, 92,104, 95, 96, 64,
 46, 32, 41, 37, 48, 69, 40, 27,
 59, 58, 43, 49, 49, 50, 42, 46,
  0,  0,  0,  0,  0,  0,  0,  0,
};
int PawnSquareValuesEndGame[64] = { 
  0,  0,  0,  0,  0,  0,  0,  0,
107,102,111, 74, 84, 84, 83, 78,
 97, 99, 88, 90, 89, 80, 78, 78,
110,107, 93, 79, 82, 76, 89, 87,
117,106, 93, 69, 73, 75, 92,101,
116,105, 72, 41, 25, 59, 83, 90,
 73, 65, 33, 27, 33, 31, 47, 60,
  0,  0,  0,  0,  0,  0,  0,  0,
};
int KnightSquareValues[64] = {
-52,-39,-35,-29,-24,-32,-41,-52,
-39,-30,-21,-15,-13, -4,-25,-35,
-37,-19, -7,  5,  4, -3, -1,-37,
-25, -7,  9, 12, 12,  8,  5,-28,
-20,  0, 17, 30, 25, 24,  1,-13,
-37, -7,  8, 18, 15, 14, -1,-29,
-50,-21,  5,  5, -8, -6,-24,-46,
-73,-45,-30,-37,-26,-44,-43,-61,
};
int BishopSquareValues[64] = {
 -7,-13,  0, 12,-13,-31,-18,-27,
-22, -8,  8,  2,  6, -4,-12, -6,
-10,  0,  1, -3,  0, -1,  5, -3,
-10, -8, -3, -2,  5,  3,  2,  4,
-17,-16, -3, -1, 15,  7, 10, -6,
-19,-11,  1, 10, 23, 21, 14, 12,
-26,-24,  0,  7,  8, 14,  5,  2,
-28,  3,  4, -1,  9,  2, -5,  2,
};
int RookSquareValues[64] = {
 -9, -9, -3,  2,  0, -2,-17,-13,
-21,-11,-15, -8,-13, -5,-12,-33,
-19, -9,-15,-16,-12, -9, -6,-18,
-12, -4, -8, -7,-10, -6, -5, -9,
 -4, -3,  4, -3,  2,  9, -2, -2,
  1,  5,  4,  5, -6,  5,  4, -2,
  0,  1,  4,  1, -7,  4,  0, -4,
 12,  7, 10, 10, 12,  7,  6, 10,
};
int QueenSquareValues[64] = {
 -7,-13,  0, 12,-13,-31,-18,-27,
-22, -8,  8,  2,  6, -4,-12, -6,
-10,  0,  1, -3,  0, -1,  5, -3,
-10, -8, -3, -2,  5,  3,  2,  4,
-17,-16, -3, -1, 15,  7, 10, -6,
-19,-11,  1, 10, 23, 21, 14, 12,
-26,-24,  0,  7,  8, 14,  5,  2,
-28,  3,  4, -1,  9,  2, -5,  2,
};
int KingSquareMid[64] = {
 20, 91, 74,-15, 40, 23,110, 90,
 27, 44, 31,  0, 18, 49,113,103,
 -5,-10, -8, -6,  5, 19, 43, 21,
-20,-22,-27,-33,-29,-16,-15,-10,
-27,-31,-35,-47,-46,-32,-25,-25,
-25,-34,-36,-47,-48,-30,-27,-23,
-29,-34,-38,-47,-50,-32,-36,-28,
-30,-40,-39,-49,-50,-40,-40,-30,
};
int KingSquareEndGame[64] = {
-59,-26,-12,  0, -6,  9,-23,-43,
-27, -1, 24, 26, 40, 29,  1,-12,
-22, 15, 32, 44, 51, 42, 33,  9,
-29,  6, 42, 51, 55, 55, 37, -7,
-24, 25, 42, 46, 48, 50, 40,  3,
-18, 11, 27, 30, 23, 49, 37, -5,
-28,-10,  8,  7, 10, 20,-10,-18,
-53,-30,-27,-31,-28,-24,-26,-49,
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
