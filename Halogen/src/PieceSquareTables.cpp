#include "PieceSquareTables.h"
#include <iostream>

int PieceSquareTables[N_STAGES][N_PIECES][N_SQUARES];
void AddPieceSquareTableLayer(unsigned int stage, unsigned int piece, int layer[64]);			

int PawnSquareValuesMid[64] = {
0,0,0,0,0,0,0,0,-25,-11,-25,-29,-22,28,29,-10,-8,-3,-2,-11,3,6,33,-4,-11,2,0,16,15,14,12,-13,5,20,9,26,38,18,20,-7,14,15,21,20,28,27,13,11,53,53,48,47,50,49,47,47,0,0,0,0,0,0,0,0
};
int PawnSquareValuesEndGame[64] = {
0,0,0,0,0,0,0,0,20,20,22,5,16,13,12,4,11,19,7,14,15,10,6,4,27,18,7,1,3,4,15,16,33,28,14,-5,4,8,25,27,52,47,32,4,9,20,33,38,44,40,28,24,30,29,35,41,35,35,35,35,35,35,35,35
};
int KnightSquareValues[64] = {
-52,-35,-34,-29,-25,-30,-34,-51,-39,-21,-8,-7,-7,2,-21,-37,-32,-4,6,11,15,6,4,-29,-24,-1,12,17,18,15,2,-28,-26,5,18,31,26,22,7,-22,-31,2,9,18,18,12,4,-30,-44,-22,0,0,0,-1,-22,-40,-56,-40,-30,-32,-30,-31,-41,-52

};
int BishopSquareValues[64] = {
-24,-10,-21,-12,-10,-20,-11,-19,-7,5,2,-2,4,4,20,-11,-11,4,11,12,13,14,13,-9,-12,3,9,16,11,11,1,-8,-7,0,5,13,16,9,0,-11,-9,1,4,11,8,9,2,-9,-9,0,-1,-1,-1,2,1,-15,-20,-12,-13,-11,-11,-12,-12,-19

};
int RookSquareValues[64] = {
-3,-3,4,6,5,6,-14,-12,-17,-6,-6,-6,-5,-1,-4,-19,-12,-1,-5,-5,-5,-5,-3,-11,-4,-1,1,-1,-4,-2,1,-7,-1,2,6,2,3,2,-1,-4,2,6,2,2,-2,0,5,-2,13,21,20,20,14,21,19,11,9,6,6,5,7,4,2,4

};
int QueenSquareValues[64] = {
-16,-12,-9,8,-8,-14,-11,-21,-14,-2,10,1,1,-2,-2,-10,-7,-2,2,1,3,4,4,-8,-9,-5,4,1,11,7,2,2,-8,-5,3,4,8,7,2,-4,-12,-5,1,7,10,11,2,-3,-16,-7,1,1,2,3,0,-6,-24,-8,-10,-4,-3,-9,-10,-17

};
int KingSquareMid[64] = {
18,33,15,-15,9,-7,42,25,19,22,4,-8,-7,-2,25,25,-10,-18,-19,-22,-24,-22,-14,-8,-21,-29,-30,-42,-42,-30,-28,-18,-30,-39,-41,-51,-51,-39,-38,-28,-29,-39,-40,-50,-51,-39,-38,-29,-30,-39,-40,-50,-51,-38,-39,-30,-30,-40,-40,-50,-50,-40,-40,-30

};
int KingSquareEndGame[64] = {
-55,-41,-30,-27,-34,-30,-44,-63,-32,-22,-4,-1,5,0,-14,-32,-29,-9,16,20,22,14,3,-19,-30,-9,25,31,30,26,-1,-28,-29,-4,27,34,34,31,2,-23,-28,-5,19,27,26,25,-2,-26,-31,-27,1,0,1,5,-27,-29,-51,-30,-29,-30,-31,-29,-30,-50
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
