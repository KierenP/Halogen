#include "BitBoardDefine.h"

uint64_t EMPTY;
uint64_t UNIVERCE;

uint64_t RankBB[N_RANKS];
uint64_t FileBB[N_FILES];
uint64_t SquareBB[N_SQUARES];
uint64_t DiagonalBB[N_DIAGONALS];
uint64_t AntiDiagonalBB[N_ANTI_DIAGONALS];

uint64_t betweenArray[64][64];

uint64_t RookAttacks[N_SQUARES];
uint64_t KnightAttacks[N_SQUARES];
uint64_t BishopAttacks[N_SQUARES];
uint64_t QueenAttacks[N_SQUARES];
uint64_t KingAttacks[N_SQUARES];
uint64_t WhitePawnAttacks[N_SQUARES];
uint64_t BlackPawnAttacks[N_SQUARES];

bool HASH_ENABLE = true;

const int index64[64] = {
	0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};

Players operator!(const Players& val)
{
	return val == WHITE ? BLACK : WHITE;
}

//--------------------------------------------------------------------------
//Below code adapted with permission from Terje, author of Weiss.

uint64_t BishopAttacksMagic[0x1480];
uint64_t RookAttacksMagic[0x19000];
void InitSliderAttacks(Magic m[64], uint64_t* table, const int steps[4]);

//--------------------------------------------------------------------------

void BBInit()
{
	UNIVERCE = 0xffffffffffffffff;
	EMPTY = 0;

	//A loop didnt work, and if these have a problem debugging is hard so they were hard coded
	RankBB[RANK_1] = 0xff;
	RankBB[RANK_2] = 0xff00;
	RankBB[RANK_3] = 0xff0000;
	RankBB[RANK_4] = 0xff000000;
	RankBB[RANK_5] = 0xff00000000;
	RankBB[RANK_6] = 0xff0000000000;
	RankBB[RANK_7] = 0xff000000000000;
	RankBB[RANK_8] = 0xff00000000000000;

	FileBB[FILE_H] = 0x8080808080808080;
	FileBB[FILE_G] = 0x4040404040404040;
	FileBB[FILE_F] = 0x2020202020202020;
	FileBB[FILE_E] = 0x1010101010101010;
	FileBB[FILE_D] = 0x808080808080808;
	FileBB[FILE_C] = 0x404040404040404;
	FileBB[FILE_B] = 0x202020202020202;
	FileBB[FILE_A] = 0x101010101010101;

	DiagonalBB[DIAG_A1H8] = 0x8040201008040201;
	DiagonalBB[DIAG_A2G8] = 0x4020100804020100;
	DiagonalBB[DIAG_A3F8] = 0x2010080402010000;
	DiagonalBB[DIAG_A4E8] = 0x1008040201000000;
	DiagonalBB[DIAG_A5D8] = 0x804020100000000;
	DiagonalBB[DIAG_A6C8] = 0x402010000000000;
	DiagonalBB[DIAG_A7B8] = 0x201000000000000;
	DiagonalBB[DIAG_A8A8] = 0x100000000000000;
	DiagonalBB[DIAG_B1H7] = 0x80402010080402;
	DiagonalBB[DIAG_C1H6] = 0x804020100804;
	DiagonalBB[DIAG_D1H5] = 0x8040201008;
	DiagonalBB[DIAG_E1H4] = 0x80402010;
	DiagonalBB[DIAG_F1H3] = 0x804020;
	DiagonalBB[DIAG_G1H2] = 0x8040;
	DiagonalBB[DIAG_H1H1] = 0x80;

	AntiDiagonalBB[DIAG_H8H8] = 0x8000000000000000;
	AntiDiagonalBB[DIAG_G8H7] = 0x4080000000000000;
	AntiDiagonalBB[DIAG_F8H6] = 0x2040800000000000;
	AntiDiagonalBB[DIAG_E8H5] = 0x1020408000000000;
	AntiDiagonalBB[DIAG_D8H4] = 0x810204080000000;
	AntiDiagonalBB[DIAG_C8H3] = 0x408102040800000;
	AntiDiagonalBB[DIAG_B8H2] = 0x204081020408000;
	AntiDiagonalBB[DIAG_A8H1] = 0x102040810204080;
	AntiDiagonalBB[DIAG_A7G1] = 0x1020408102040;
	AntiDiagonalBB[DIAG_A6F1] = 0x10204081020;
	AntiDiagonalBB[DIAG_A5E1] = 0x102040810;
	AntiDiagonalBB[DIAG_A4D1] = 0x1020408;
	AntiDiagonalBB[DIAG_A3C1] = 0x10204;
	AntiDiagonalBB[DIAG_A2B1] = 0x102;
	AntiDiagonalBB[DIAG_A1A1] = 0x1;

	for (int i = SQ_A1; i < N_SQUARES; i++)
	{
		uint64_t Bit = 1;
		SquareBB[i] = Bit << i;
	}

	for (int i = 0; i < 64; i++)
	{
		for (int j = 0; j < 64; j++)
		{
			betweenArray[i][j] = inBetween(i, j);
		}
	}

	for (int i = 0; i < 64; i++)
	{
		KingAttacks[i] = EMPTY;
		WhitePawnAttacks[i] = EMPTY;
		BlackPawnAttacks[i] = EMPTY;

		for (int j = 0; j < 64; j++)
		{
			if (AbsFileDiff(i, j) <= 1 && AbsRankDiff(i, j) <= 1)
			{
				if (i != j)
				{
					KingAttacks[i] ^= SquareBB[j];
				}
			}

			if ((AbsFileDiff(i, j) == 1) && RankDiff(j, i) == 1)		//either side one ahead
			{
				WhitePawnAttacks[i] ^= SquareBB[j];
			}
			if ((AbsFileDiff(i, j) == 1) && RankDiff(j, i) == -1)		//either side one behind
			{
				BlackPawnAttacks[i] ^= SquareBB[j];
			}

			if ((AbsRankDiff(i, j) == 1 && AbsFileDiff(i, j) == 2) || (AbsRankDiff(i, j) == 2 && AbsFileDiff(i, j) == 1))
				KnightAttacks[i] |= SquareBB[j];
		}

		RookAttacks[i] = (RankBB[GetRank(i)] | FileBB[GetFile(i)]) ^ SquareBB[i];		//All the spaces in the same rank and file but not the starting square
		BishopAttacks[i] = (DiagonalBB[GetDiagonal(i)] | AntiDiagonalBB[GetAntiDiagonal(i)]) ^ SquareBB[i];
		QueenAttacks[i] = RookAttacks[i] | BishopAttacks[i];
 	}

	const int BSteps[4] = { 7, 9, -7, -9 };
	const int RSteps[4] = { 8, 1, -8, -1 };

	InitSliderAttacks(BishopTable, BishopAttacksMagic, BSteps);
	InitSliderAttacks(RookTable, RookAttacksMagic, RSteps);
}

char PieceToChar(unsigned int piece)
{
	assert(piece <= N_PIECES);

	char PieceChar[13] = { 'p', 'n', 'b', 'r', 'q', 'k', 'P', 'N', 'B', 'R', 'Q', 'K', ' ' };
	return PieceChar[piece];
}

unsigned int Piece(unsigned int piecetype, unsigned int colour)
{
	assert(piecetype < N_PIECE_TYPES);
	assert(colour < N_PLAYERS);

	return piecetype + N_PIECE_TYPES * colour;
}

Pieces Piece(PieceTypes type, Players colour)
{
	assert(type < N_PIECE_TYPES);
	assert(colour < N_PLAYERS);

	return static_cast<Pieces>(type + N_PIECE_TYPES * colour);
}

Square GetPosition(File file, Rank rank)
{
	assert(file < N_FILES);
	assert(rank < N_RANKS);

	return static_cast<Square>(rank * 8 + file);
}

File GetFile(Square square)
{
	assert(square < N_SQUARES);
	return static_cast<File>(square % 8);
}

Rank GetRank(Square square)
{
	assert(square < N_SQUARES);
	return static_cast<Rank>(square / 8);
}

unsigned int GetFile(unsigned int square)
{
	assert(square < N_SQUARES);

	return square % 8;
}

unsigned int GetRank(unsigned int square)
{
	assert(square < N_SQUARES);

	return square / 8;
}

unsigned int GetPosition(unsigned int file, unsigned int rank)
{
	assert(file < N_FILES);
	assert(file < N_RANKS);

	return rank * 8 + file;
}

unsigned int AbsRankDiff(unsigned int sq1, unsigned int sq2)
{
	assert(sq1 < N_SQUARES);
	assert(sq2 < N_SQUARES);

	return abs(static_cast<int>(GetRank(sq1)) - static_cast<int>(GetRank(sq2)));
}

unsigned int AbsFileDiff(unsigned int sq1, unsigned int sq2)
{
	assert(sq1 < N_SQUARES);
	assert(sq2 < N_SQUARES);

	return abs(static_cast<int>(GetFile(sq1)) - static_cast<int>(GetFile(sq2)));
}

int RankDiff(unsigned int sq1, unsigned int sq2)
{
	assert(sq1 < N_SQUARES);
	assert(sq2 < N_SQUARES);

	return static_cast<int>(GetRank(sq1)) - static_cast<int>(GetRank(sq2));
}

int FileDiff(unsigned int sq1, unsigned int sq2)
{
	assert(sq1 < N_SQUARES);
	assert(sq2 < N_SQUARES);

	return static_cast<int>(GetFile(sq1)) - static_cast<int>(GetFile(sq2));
}

unsigned int GetDiagonal(unsigned int square)
{
	assert(square < N_SQUARES);

	return (RANK_8 - GetRank(square)) + GetFile(square);
}

unsigned int GetAntiDiagonal(unsigned int square)
{
	assert(square < N_SQUARES);

	return RANK_8 + FILE_H - GetRank(square) - GetFile(square);
}

unsigned int GetBitCount(uint64_t bb)
{
#if defined(_MSC_VER) && defined(USE_POPCNT) && defined(_WIN64)
	return __popcnt64(bb);
#elif defined(__GNUG__) && defined(USE_POPCNT)
	return __builtin_popcountll(bb);
#else
	return std::bitset<std::numeric_limits<uint64_t>::digits>(bb).count();
#endif
}

unsigned int AlgebraicToPos(std::string str)
{
	if (str == "-")
		return N_SQUARES;

	assert(str.length() >= 2);

	return (str[0] - 97) + (str[1] - 49) * 8;		
}

unsigned int ColourOfPiece(unsigned int piece)
{
	assert(piece < N_PIECES);

	return piece / N_PIECE_TYPES;
}

int LSBpop(uint64_t &bb)
{
	assert(bb != 0);

	int index = LSB(bb);

	bb &= bb - 1;
	return index;
}

int LSB(uint64_t bb)
{
#if defined(_MSC_VER) && defined(USE_POPCNT) && defined(_WIN64)
	unsigned long index;
	_BitScanForward64(&index, bb);
	return index;
#elif defined(__GNUG__) && defined(USE_POPCNT)
	return __builtin_ctzll(bb);
#else
	/**
	 * bitScanForward
	 * @author Kim Walisch (2012)
	 * @param bb bitboard to scan
	 * @precondition bb != 0
	 * @return index (0..63) of least significant one bit
	 */
	const uint64_t debruijn64 = uint64_t(0x03f79d71b4cb0a89);
	return index64[((bb ^ (bb - 1)) * debruijn64) >> 58];
#endif
}

//Not my code
uint64_t inBetween(unsigned int sq1, unsigned int sq2)
{
	const uint64_t a2a7 = uint64_t(0x0001010101010100);
	const uint64_t b2g7 = uint64_t(0x0040201008040200);
	const uint64_t h1b7 = uint64_t(0x0002040810204080); /* Thanks Dustin, g2b7 did not work for c1-a3 */
	int64_t btwn, line, rank, file;

	btwn = (UNIVERCE << sq1) ^ (UNIVERCE << sq2);
	file = (sq2 & 7) - (sq1 & 7);
	rank = ((sq2 | 7) - sq1) >> 3;
	line = ((file & 7) - 1) & a2a7;					/* a2a7 if same file */
	line += 2 * (((rank & 7) - 1) >> 58);			/* b1g1 if same rank */
	line += (((rank - file) & 15) - 1) & b2g7;		/* b2g7 if same diagonal */
	line += (((rank + file) & 15) - 1) & h1b7;		/* h1b7 if same antidiag */
	line = int64_t(uint64_t(line) << (std::min)(sq1, sq2));							
	return line & btwn;								/* return the bits on that line in-between */
}

uint64_t inBetweenCache(unsigned int from, unsigned int to)
{
	assert(from < N_SQUARES);
	assert(to < N_SQUARES);

	return betweenArray[from][to];
}

bool mayMove(unsigned int from, unsigned int to, uint64_t pieces)
{
	return (inBetweenCache(from, to) & pieces) == 0;
}

//--------------------------------------------------------------------------
//Below code adapted with permission from Terje, author of Weiss.

Magic BishopTable[64];
Magic RookTable[64];

// Helper function that returns a bitboard with the landing square of
// the step, or an empty bitboard if the step would go outside the board
uint64_t LandingSquareBB(const int sq, const int step) {
	const unsigned int to = sq + step;
	return (uint64_t)(to <= SQ_H8 && std::max(AbsFileDiff(sq, to), AbsRankDiff(sq, to)) <= 2) << (to & SQ_H8);
}

// Helper function that makes a slider attack bitboard
uint64_t MakeSliderAttackBB(const int sq, const uint64_t occupied, const int steps[4]) {

	uint64_t attacks = 0;

	for (int dir = 0; dir < 4; ++dir) {

		int s = sq;
		while (!(occupied & SquareBB[s]) && LandingSquareBB(s, steps[dir]))
			attacks |= SquareBB[(s += steps[dir])];
	}

	return attacks;
}

// Initializes slider attack lookups
void InitSliderAttacks(Magic m[64], uint64_t *table, const int steps[4]) {

#ifndef USE_PEXT
	const uint64_t* magics = steps[0] == 8 ? RookMagics : BishopMagics;
#endif

	for (uint32_t sq = 0; sq < N_SQUARES; ++sq) {

		m[sq].attacks = table;

		// Construct the mask
		uint64_t edges = ((RankBB[RANK_1] | RankBB[RANK_8]) & ~RankBB[GetRank(sq)])
			| ((FileBB[FILE_A] | FileBB[FILE_H]) & ~FileBB[GetFile(sq)]);

		m[sq].mask = MakeSliderAttackBB(sq, 0, steps) & ~edges;

#ifndef USE_PEXT
		m[sq].magic = magics[sq];
		m[sq].shift = 64 - GetBitCount(m[sq].mask);
#endif

		uint64_t occupied = 0;
		do {
			m[sq].attacks[AttackIndex(sq, occupied, m)] = MakeSliderAttackBB(sq, occupied, steps);
			occupied = (occupied - m[sq].mask) & m[sq].mask; // Carry rippler
			table++;
		} while (occupied);
	}
}

//--------------------------------------------------------------------------