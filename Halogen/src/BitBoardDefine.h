#pragma once
#include <string>
#include <assert.h>
#include <bitset>
#include <stdexcept>
#include <limits>
#include <type_traits>
#include <algorithm>

#ifdef _MSC_VER
#include <intrin.h>
#define NOMINMAX
#endif

extern bool HASH_ENABLE;

enum Square
{
	SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
	SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
	SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
	SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
	SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
	SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
	SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
	SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,

	N_SQUARES,
};

enum Players
{
	BLACK,
	WHITE,

	N_PLAYERS
};

Players operator!(const Players& val);

enum Pieces
{
	BLACK_PAWN,
	BLACK_KNIGHT,
	BLACK_BISHOP,
	BLACK_ROOK,
	BLACK_QUEEN,
	BLACK_KING,

	WHITE_PAWN,
	WHITE_KNIGHT,
	WHITE_BISHOP,
	WHITE_ROOK,
	WHITE_QUEEN,
	WHITE_KING,

	N_PIECES
};

enum PieceTypes
{
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,

	N_PIECE_TYPES
};

enum Rank
{
	RANK_1,
	RANK_2,
	RANK_3,
	RANK_4,
	RANK_5,
	RANK_6,
	RANK_7,
	RANK_8,

	N_RANKS
};

enum File
{
	FILE_A,
	FILE_B,
	FILE_C,
	FILE_D,
	FILE_E,
	FILE_F,
	FILE_G,
	FILE_H,

	N_FILES
};

enum Diagonal
{
	DIAG_A8A8,
	DIAG_A7B8,
	DIAG_A6C8,
	DIAG_A5D8,
	DIAG_A4E8,
	DIAG_A3F8,
	DIAG_A2G8,
	DIAG_A1H8,
	DIAG_B1H7,
	DIAG_C1H6,
	DIAG_D1H5,
	DIAG_E1H4,
	DIAG_F1H3,
	DIAG_G1H2,
	DIAG_H1H1,

	N_DIAGONALS
};

enum AnitDiagonal
{
	DIAG_H8H8,
	DIAG_G8H7,
	DIAG_F8H6,
	DIAG_E8H5,
	DIAG_D8H4,
	DIAG_C8H3,
	DIAG_B8H2,
	DIAG_A8H1,
	DIAG_A7G1,
	DIAG_A6F1,
	DIAG_A5E1,
	DIAG_A4D1,
	DIAG_A3C1,
	DIAG_A2B1,
	DIAG_A1A1,

	N_ANTI_DIAGONALS
};

enum GameStages
{
	MIDGAME,		
	ENDGAME,
	N_STAGES
};

void BBInit();
char PieceToChar(unsigned int piece);
unsigned int Piece(unsigned int piecetype, unsigned int colour);

unsigned int GetFile(unsigned int square);
unsigned int GetRank(unsigned int square);
unsigned int GetPosition(unsigned int file, unsigned int rank);
unsigned int AbsRankDiff(unsigned int sq1, unsigned int sq2);
unsigned int AbsFileDiff(unsigned int sq1, unsigned int sq2);
int RankDiff(unsigned int sq1, unsigned int sq2);
int FileDiff(unsigned int sq1, unsigned int sq2);
unsigned int GetDiagonal(unsigned int square);
unsigned int GetAntiDiagonal(unsigned int square);
unsigned int GetBitCount(uint64_t bb);
unsigned int AlgebraicToPos(std::string str);
unsigned int ColourOfPiece(unsigned int piece);

//TODO slowly change over the functions to use expicit enums
Pieces Piece(PieceTypes type, Players colour);
Square GetPosition(File file, Rank rank);
File GetFile(Square square);
Rank GetRank(Square square);

extern uint64_t EMPTY;
extern uint64_t UNIVERCE;

extern uint64_t RankBB[N_RANKS];
extern uint64_t FileBB[N_FILES];
extern uint64_t SquareBB[N_SQUARES];
extern uint64_t DiagonalBB[N_DIAGONALS];
extern uint64_t AntiDiagonalBB[N_ANTI_DIAGONALS];
extern uint64_t PassedPawnMaskWhite[N_SQUARES];
extern uint64_t PassedPawnMaskBlack[N_SQUARES];

extern const int index64[64];
extern uint64_t betweenArray[64][64];

extern uint64_t KnightAttacks[N_SQUARES];
extern uint64_t RookAttacks[N_SQUARES];
extern uint64_t BishopAttacks[N_SQUARES];
extern uint64_t QueenAttacks[N_SQUARES];
extern uint64_t KingAttacks[N_SQUARES];
extern uint64_t WhitePawnAttacks[N_SQUARES];
extern uint64_t BlackPawnAttacks[N_SQUARES];

extern uint64_t allBitsBelow[N_SQUARES];
extern uint64_t allBitsAbove[N_SQUARES];

int LSPpop(uint64_t &bb);
int LSB(uint64_t bb);

uint64_t inBetween(unsigned int sq1, unsigned int sq2);	//return the bb of the squares in between (exclusive) the two squares
uint64_t inBetweenCache(unsigned int from, unsigned int to);
bool mayMove(unsigned int from, unsigned int to, uint64_t pieces);

const unsigned int MAX_DEPTH = 100;
