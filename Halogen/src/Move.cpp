#include "Move.h"


void PrintMove(Move move, std::ostream& ss)
{
	Square prev    = GetFrom(move);
	Square current = GetTo(move);

	ss << (char)(GetFile(prev) + 'a') << GetRank(prev) + 1 << (char)(GetFile(current) + 'a') << GetRank(current) + 1;	//+1 to make it from 1-8 and not 0-7

	if (isPromotion(move))
	{
		if (GetFlag(move) == KNIGHT_PROMOTION || GetFlag(move) == KNIGHT_PROMOTION_CAPTURE)
			ss << "n";
		if (GetFlag(move) == BISHOP_PROMOTION || GetFlag(move) == BISHOP_PROMOTION_CAPTURE)
			ss << "b";
		if (GetFlag(move) == QUEEN_PROMOTION || GetFlag(move) == QUEEN_PROMOTION_CAPTURE)
			ss << "q";
		if (GetFlag(move) == ROOK_PROMOTION || GetFlag(move) == ROOK_PROMOTION_CAPTURE)
			ss << "r";
	}
}