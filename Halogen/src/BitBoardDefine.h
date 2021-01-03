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

extern const int index64[64];
extern uint64_t betweenArray[64][64];

extern uint64_t KnightAttacks[N_SQUARES];
extern uint64_t RookAttacks[N_SQUARES];
extern uint64_t BishopAttacks[N_SQUARES];
extern uint64_t QueenAttacks[N_SQUARES];
extern uint64_t KingAttacks[N_SQUARES];
extern uint64_t WhitePawnAttacks[N_SQUARES];
extern uint64_t BlackPawnAttacks[N_SQUARES];

int LSBpop(uint64_t &bb);
int LSB(uint64_t bb);

uint64_t inBetween(unsigned int sq1, unsigned int sq2);	//return the bb of the squares in between (exclusive) the two squares
uint64_t inBetweenCache(unsigned int from, unsigned int to);
bool mayMove(unsigned int from, unsigned int to, uint64_t pieces);

const unsigned int MAX_DEPTH = 100;

//--------------------------------------------------------------------------
//Below code adapted with permission from Terje, author of Weiss.

#ifdef USE_PEXT
// Uses the bmi2 pext instruction in place of magic bitboards
#include "x86intrin.h"
#define AttackIndex(sq, occ, table) (_pext_u64(occ, table[sq].mask))

#else
// Uses magic bitboards as explained on https://www.chessprogramming.org/Magic_Bitboards
#define AttackIndex(sq, occ, table) (((occ & table[sq].mask) * table[sq].magic) >> table[sq].shift)

static const uint64_t RookMagics[64] = {
	0xA180022080400230ull, 0x0040100040022000ull, 0x0080088020001002ull, 0x0080080280841000ull,
	0x4200042010460008ull, 0x04800A0003040080ull, 0x0400110082041008ull, 0x008000A041000880ull,
	0x10138001A080C010ull, 0x0000804008200480ull, 0x00010011012000C0ull, 0x0022004128102200ull,
	0x000200081201200Cull, 0x202A001048460004ull, 0x0081000100420004ull, 0x4000800380004500ull,
	0x0000208002904001ull, 0x0090004040026008ull, 0x0208808010002001ull, 0x2002020020704940ull,
	0x8048010008110005ull, 0x6820808004002200ull, 0x0A80040008023011ull, 0x00B1460000811044ull,
	0x4204400080008EA0ull, 0xB002400180200184ull, 0x2020200080100380ull, 0x0010080080100080ull,
	0x2204080080800400ull, 0x0000A40080360080ull, 0x02040604002810B1ull, 0x008C218600004104ull,
	0x8180004000402000ull, 0x488C402000401001ull, 0x4018A00080801004ull, 0x1230002105001008ull,
	0x8904800800800400ull, 0x0042000C42003810ull, 0x008408110400B012ull, 0x0018086182000401ull,
	0x2240088020C28000ull, 0x001001201040C004ull, 0x0A02008010420020ull, 0x0010003009010060ull,
	0x0004008008008014ull, 0x0080020004008080ull, 0x0282020001008080ull, 0x50000181204A0004ull,
	0x48FFFE99FECFAA00ull, 0x48FFFE99FECFAA00ull, 0x497FFFADFF9C2E00ull, 0x613FFFDDFFCE9200ull,
	0xFFFFFFE9FFE7CE00ull, 0xFFFFFFF5FFF3E600ull, 0x0010301802830400ull, 0x510FFFF5F63C96A0ull,
	0xEBFFFFB9FF9FC526ull, 0x61FFFEDDFEEDAEAEull, 0x53BFFFEDFFDEB1A2ull, 0x127FFFB9FFDFB5F6ull,
	0x411FFFDDFFDBF4D6ull, 0x0801000804000603ull, 0x0003FFEF27EEBE74ull, 0x7645FFFECBFEA79Eull,
};

static const uint64_t BishopMagics[64] = {
	0xFFEDF9FD7CFCFFFFull, 0xFC0962854A77F576ull, 0x5822022042000000ull, 0x2CA804A100200020ull,
	0x0204042200000900ull, 0x2002121024000002ull, 0xFC0A66C64A7EF576ull, 0x7FFDFDFCBD79FFFFull,
	0xFC0846A64A34FFF6ull, 0xFC087A874A3CF7F6ull, 0x1001080204002100ull, 0x1810080489021800ull,
	0x0062040420010A00ull, 0x5028043004300020ull, 0xFC0864AE59B4FF76ull, 0x3C0860AF4B35FF76ull,
	0x73C01AF56CF4CFFBull, 0x41A01CFAD64AAFFCull, 0x040C0422080A0598ull, 0x4228020082004050ull,
	0x0200800400E00100ull, 0x020B001230021040ull, 0x7C0C028F5B34FF76ull, 0xFC0A028E5AB4DF76ull,
	0x0020208050A42180ull, 0x001004804B280200ull, 0x2048020024040010ull, 0x0102C04004010200ull,
	0x020408204C002010ull, 0x02411100020080C1ull, 0x102A008084042100ull, 0x0941030000A09846ull,
	0x0244100800400200ull, 0x4000901010080696ull, 0x0000280404180020ull, 0x0800042008240100ull,
	0x0220008400088020ull, 0x04020182000904C9ull, 0x0023010400020600ull, 0x0041040020110302ull,
	0xDCEFD9B54BFCC09Full, 0xF95FFA765AFD602Bull, 0x1401210240484800ull, 0x0022244208010080ull,
	0x1105040104000210ull, 0x2040088800C40081ull, 0x43FF9A5CF4CA0C01ull, 0x4BFFCD8E7C587601ull,
	0xFC0FF2865334F576ull, 0xFC0BF6CE5924F576ull, 0x80000B0401040402ull, 0x0020004821880A00ull,
	0x8200002022440100ull, 0x0009431801010068ull, 0xC3FFB7DC36CA8C89ull, 0xC3FF8A54F4CA2C89ull,
	0xFFFFFCFCFD79EDFFull, 0xFC0863FCCB147576ull, 0x040C000022013020ull, 0x2000104000420600ull,
	0x0400000260142410ull, 0x0800633408100500ull, 0xFC087E8E4BB2F736ull, 0x43FF9E4EF4CA2C89ull,
};
#endif

struct Magic
{
	uint64_t* attacks;
	uint64_t mask;
#ifndef USE_PEXT
	uint64_t magic;
	int shift;
#endif
};

extern Magic BishopTable[64];
extern Magic RookTable[64];

//--------------------------------------------------------------------------
