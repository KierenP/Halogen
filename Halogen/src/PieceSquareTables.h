#pragma once
#include "BitBoardDefine.h"

extern int PieceSquareTables[N_STAGES][N_PIECES][N_SQUARES];
extern int PawnSquareValuesMid[64];
extern int PawnSquareValuesEndGame[64];
extern int KnightSquareValues[64];
extern int BishopSquareValues[64];
extern int RookSquareValues[64];
extern int QueenSquareValues[64];
extern int KingSquareMid[64];
extern int KingSquareEndGame[64];

void InitializePieceSquareTable();