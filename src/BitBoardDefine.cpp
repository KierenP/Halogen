#include "BitBoardDefine.h"

Players operator!(const Players& val)
{
	return val == WHITE ? BLACK : WHITE;
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

unsigned int GetPosition(unsigned int file, unsigned int rank)
{
	assert(file < N_FILES);
	assert(file < N_RANKS);

	return rank * 8 + file;
}

int GetBitCount(uint64_t bb)
{
#if defined(_MSC_VER) && defined(USE_POPCNT) && defined(_WIN64)
	return __popcnt64(bb);
#elif defined(__GNUG__) && defined(USE_POPCNT)
	return __builtin_popcountll(bb);
#else
	return std::bitset<std::numeric_limits<uint64_t>::digits>(bb).count();
#endif
}

unsigned int AlgebraicToPos(const std::string &str)
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
	static constexpr std::array<int, N_SQUARES> index64 = {
		0, 47,  1, 56, 48, 27,  2, 60,
	   57, 49, 41, 37, 28, 16,  3, 61,
	   54, 58, 35, 52, 50, 42, 21, 44,
	   38, 32, 29, 23, 17, 11,  4, 62,
	   46, 55, 26, 59, 40, 36, 15, 53,
	   34, 51, 20, 43, 31, 22, 10, 45,
	   25, 39, 14, 33, 19, 30,  9, 24,
	   13, 18,  8, 12,  7,  6,  5, 63
	};
	static constexpr uint64_t debruijn64 = uint64_t(0x03f79d71b4cb0a89);
	return index64[((bb ^ (bb - 1)) * debruijn64) >> 58];
#endif
}

//--------------------------------------------------------------------------
//Below code adapted with permission from Terje, author of Weiss.

// Helper function that returns a bitboard with the landing square of
// the step, or an empty bitboard if the step would go outside the board
constexpr uint64_t LandingSquareBB(const int sq, const int step) 
{
	const unsigned int to = sq + step;
	return (uint64_t)(to <= SQ_H8 && std::max(AbsFileDiff(sq, to), AbsRankDiff(sq, to)) <= 2) << (to & SQ_H8);
}

// Helper function that makes a slider attack bitboard
constexpr uint64_t MakeSliderAttackBB(Square sq, uint64_t occupied, const std::array<int, 4>& steps)
{
	uint64_t attacks = 0;

	for (int dir = 0; dir < 4; dir++) {

		int s = sq;
		while (!(occupied & SquareBB[s]) && LandingSquareBB(s, steps[dir]))
			attacks |= SquareBB[(s += steps[dir])];
	}

	return attacks;
}

template<uint64_t size>
void MagicTable<size>::InitSliderAttacks()
{
	uint64_t* attack = attacks.data();

	for (int i = 0; i < N_SQUARES; i++)
	{
		Square sq = static_cast<Square>(i);
		table[sq].attacks = attack;

		// Construct the mask
		uint64_t edges = ((RankBB[RANK_1] | RankBB[RANK_8]) & ~RankBB[GetRank(sq)])
			           | ((FileBB[FILE_A] | FileBB[FILE_H]) & ~FileBB[GetFile(sq)]);

		table[sq].mask = MakeSliderAttackBB(sq, 0, steps) & ~edges;

#ifndef USE_PEXT
		table[sq].magic = magics[sq];
		table[sq].shift = 64 - GetBitCount(table[sq].mask);
#endif

		uint64_t occupied = 0;
		do {
			AttackMask(sq, occupied) = MakeSliderAttackBB(sq, occupied, steps);
			occupied = (occupied - table[sq].mask) & table[sq].mask; // Carry rippler
			attack++;
		} while (occupied);
	}
}
//--------------------------------------------------------------------------