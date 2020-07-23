#include "PieceSquareTables.h"
#include <iostream>

int PieceSquareTables[N_STAGES][N_PIECES][N_SQUARES];
void AddPieceSquareTableLayer(unsigned int stage, unsigned int piece, int layer[64]);			

int PawnSquareValuesMid[64] = {
0,0,0,0,0,0,0,0,-29,-13,-27,-30,-22,29,24,-16,-16,-5,-6,-11,0,5,29,-6,-17,-7,-4,16,19,14,8,-20,0,20,10,30,35,24,25,-10,11,13,18,15,28,33,15,8,52,54,44,47,50,49,46,46,0,0,0,0,0,0,0,0
};
int PawnSquareValuesEndGame[64] = { 
0,0,0,0,0,0,0,0,18,18,20,10,19,5,7,1,13,15,6,11,11,5,2,3,25,20,4,-2,0,1,10,10,36,26,15,-3,-2,8,20,23,54,52,31,3,5,20,40,40,46,47,24,22,30,28,33,44,0,0,0,0,0,0,0,0
};
int KnightSquareValues[64] = {
-52,-38,-33,-27,-26,-31,-37,-51,-39,-25,-9,-15,-8,1,-20,-37,-36,-10,0,10,14,5,2,-28,-27,-1,13,18,18,15,2,-22,-26,7,21,33,30,27,3,-21,-32,2,13,23,17,17,4,-28,-47,-21,1,5,-2,-3,-21,-40,-59,-41,-28,-34,-29,-33,-41,-55
};
int BishopSquareValues[64] = {
-26,-8,-24,-11,-10,-25,-11,-20,-5,1,2,-5,4,5,19,-12,-14,2,9,14,12,14,5,-4,-13,3,7,18,13,12,0,-10,-7,0,11,16,20,14,3,-9,-10,1,7,5,8,12,6,-7,-10,-2,-4,-2,0,2,0,-21,-21,-13,-15,-11,-11,-13,-13,-20
};
int RookSquareValues[64] = {
-4,-1,5,6,5,5,-15,-15,-20,-9,-9,-3,-9,-3,-8,-27,-14,-8,-7,-9,-6,-5,-5,-14,-7,-1,2,-5,-4,-1,1,-8,-3,0,6,2,3,3,0,-1,4,7,2,5,-2,2,8,0,-2,1,4,3,-7,7,2,-6,12,9,11,8,5,6,3,9
};
int QueenSquareValues[64] = {
-13,-11,-8,8,-12,-19,-14,-23,-17,-6,6,-4,3,-1,-4,-8,-9,0,-1,-2,0,2,5,-7,-10,-4,-1,0,10,9,2,4,-9,-12,3,2,10,10,6,-4,-13,-5,3,9,14,14,5,2,-20,-14,-1,0,3,6,1,-2,-26,-6,-5,-3,-2,-6,-10,-16
};
int KingSquareMid[64] = {
17,38,18,-21,4,-21,50,26,18,22,4,-13,-16,-7,29,28,-10,-18,-20,-23,-26,-24,-12,-8,-21,-28,-32,-44,-43,-29,-27,-19,-31,-38,-40,-51,-52,-38,-36,-29,-28,-38,-40,-50,-50,-38,-36,-27,-30,-38,-39,-49,-51,-36,-38,-30,-30,-40,-40,-50,-50,-40,-40,-30
};
int KingSquareEndGame[64] = {
-58,-45,-34,-28,-41,-30,-55,-69,-34,-23,-3,-1,5,2,-13,-34,-30,-6,10,14,19,16,3,-21,-31,-10,20,24,25,26,7,-27,-31,0,26,33,30,31,10,-19,-27,-2,19,25,23,30,6,-23,-31,-22,2,1,2,8,-25,-28,-52,-30,-29,-31,-31,-29,-29,-50
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
