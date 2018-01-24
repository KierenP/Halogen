#include "PieceSquareTables.h"

int PieceSquareTables[N_STAGES][N_PIECES][N_SQUARES];
void AddPieceSquareTableLayer(unsigned int stage, unsigned int piece, const int layer[64]);			

const int PawnSquareValuesOpeningMid[64] = {
	0,  0,  0,  0,  0,  0,  0,  0,
	5, 10, 10,-20,-20, 10, 10,  5,
	5, -5,-10,  0,  0,-10, -5,  5,
	0,  0,  0, 20, 20,  0,  0,  0,
	5,  5, 10, 25, 25, 10,  5,  5,
	10, 10, 20, 30, 30, 20, 10, 10,
	50, 50, 50, 50, 50, 50, 50, 50,
	0,  0,  0,  0,  0,  0,  0,  0
};
const int PawnSquareValuesEndGame[64] = {
	0,  0,  0,  0,  0,  0,  0,  0,
	5,  5,  5,  5,  5,  5,  5,  5,
	10, 10, 10, 10, 10, 10, 10, 10, 
	15, 15, 15, 15, 15, 15, 15, 15,
	20, 20, 20, 20, 20, 20, 20, 20,
	25, 25, 25, 25, 25, 25, 25, 25,
	30, 30, 30, 30, 30, 30, 30, 30, 
	35, 35, 35, 35, 35, 35, 35, 35,
};
const int KnightSquareValues[64] = {
	-50,-40,-30,-30,-30,-30,-40,-50,	//1
	-40,-20,  0,  5,  5,  0,-20,-40,	//2
	-30,  5, 10, 15, 15, 10,  5,-30,	//3
	-30,  0, 15, 20, 20, 15,  0,-30,	//4
	-30,  5, 15, 20, 20, 15,  5,-30,	//5
	-30,  0, 10, 15, 15, 10,  0,-30,	//6
	-40,-20,  0,  0,  0,  0,-20,-40,	//7
	-50,-40,-30,-30,-30,-30,-40,-50	    //8
};
const int BishopSquareValues[64] = {
	-20,-10,-10,-10,-10,-10,-10,-20,
	-10,  20,  0,  0,  0,  0,  20,-10,
	-10, 10, 10, 10, 10, 10, 10,-10,
	-10,  0, 10, 10, 10, 10,  0,-10,
	-10,  5,  5, 10, 10,  5,  5,-10,
	-10,  0,  5, 10, 10,  5,  0,-10,
	-10,  0,  0,  0,  0,  0,  0,-10,
	-20,-10,-10,-10,-10,-10,-10,-20
};
const int RookSquareValues[64] = {
	0,  0,  0,  0,  0,  0,  0,  0,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	5, 20, 20, 20, 20, 20, 20,  5,
	0,  0,  0,  0,  0,  0,  0,  0,
};
const int QueenSquareValues[64] = {
	-20,-10,-10, 0, -5,-10,-10,-20
	- 10,  0,  5,  0,  0,  0,  0,-10,
	-10,  5,  5,  5,  5,  5,  0,-10,
	0,  0,  5,  5,  5,  5,  0, -5,
	-5,  0,  5,  5,  5,  5,  0, -5,
	-10,  0,  5,  5,  5,  5,  0,-10,
	-10,  0,  0,  0,  0,  0,  0,-10,
	-20,-10,-10, -5, -5,-10,-10,-20
};
const int KingSquareOpeningMid[64] = {
	20, 30, 10,  0,  0, 10, 30, 20,
	20, 20,  0,  0,  0,  0, 20, 20,
	-10,-20,-20,-20,-20,-20,-20,-10,
	-20,-30,-30,-40,-40,-30,-30,-20,
	-30,-40,-40,-50,-50,-40,-40,-30,
	-30,-40,-40,-50,-50,-40,-40,-30,
	-30,-40,-40,-50,-50,-40,-40,-30,
	-30,-40,-40,-50,-50,-40,-40,-30,
};
const int KingSquareEndGame[64] = {
	-50,-40,-30,-20,-20,-30,-40,-50,
	-30,-20,-10,  0,  0,-10,-20,-30,
	-30,-10, 20, 30, 30, 20,-10,-30,
	-30,-10, 30, 40, 40, 30,-10,-30,
	-30,-10, 30, 40, 40, 30,-10,-30,
	-30,-10, 20, 30, 30, 20,-10,-30,
	-30,-30,  0,  0,  0,  0,-30,-30,
	-50,-30,-30,-30,-30,-30,-30,-50
};

void InitializePieceSquareTable()
{
	for (int i = OPENING; i < N_STAGES; i++)
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

	AddPieceSquareTableLayer(OPENING, WHITE_KING, KingSquareOpeningMid);
	AddPieceSquareTableLayer(OPENING, BLACK_KING, KingSquareOpeningMid);
	AddPieceSquareTableLayer(MIDGAME, WHITE_KING, KingSquareOpeningMid);
	AddPieceSquareTableLayer(MIDGAME, BLACK_KING, KingSquareOpeningMid);
	AddPieceSquareTableLayer(ENDGAME, WHITE_KING, KingSquareEndGame);
	AddPieceSquareTableLayer(ENDGAME, BLACK_KING, KingSquareEndGame);

	AddPieceSquareTableLayer(OPENING, WHITE_PAWN, PawnSquareValuesOpeningMid);
	AddPieceSquareTableLayer(OPENING, BLACK_PAWN, PawnSquareValuesOpeningMid);
	AddPieceSquareTableLayer(MIDGAME, WHITE_PAWN, PawnSquareValuesOpeningMid);
	AddPieceSquareTableLayer(MIDGAME, BLACK_PAWN, PawnSquareValuesOpeningMid);
	AddPieceSquareTableLayer(ENDGAME, WHITE_PAWN, PawnSquareValuesEndGame);
	AddPieceSquareTableLayer(ENDGAME, BLACK_PAWN, PawnSquareValuesEndGame);
}

void AddPieceSquareTableLayer(unsigned int stage, unsigned int piece, const int layer[64])
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
