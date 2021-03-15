#include "MoveGenerator.h"

MoveGenerator::MoveGenerator(Position& Position, int DistanceFromRoot, const SearchData& Locals, bool Quiescence) :
	position(Position), distanceFromRoot(DistanceFromRoot), locals(Locals), quiescence(Quiescence)
{
	if (quiescence)
		stage = Stage::GEN_LOUD;
	else
		stage = Stage::TT_MOVE;
}

bool MoveGenerator::Next(Move& move)
{
	if (stage == Stage::TT_MOVE)
	{
		TTmove = GetHashMove(position, distanceFromRoot);
		stage = Stage::GEN_LOUD;

		if (MoveIsLegal(position, TTmove))
		{
			move = TTmove;
			return true;
		}
	}

	if (stage == Stage::GEN_LOUD)
	{
		if (!quiescence && IsInCheck(position))
		{
			LegalMoves(position, legalMoves);	//contains a special function for generating moves when in check which is quicker
			stage = Stage::GIVE_QUIET;
		}
		else
		{
			QuiescenceMoves(position, legalMoves);
			stage = Stage::GIVE_GOOD_LOUD;
		}

		OrderMoves(legalMoves);
		current = legalMoves.begin();
	}

	if (stage == Stage::GIVE_GOOD_LOUD)
	{
		if (current != legalMoves.end() && current->SEE >= 0)
		{
			move = current->move;
			++current;
			return true;
		}

		if (quiescence)
			return false;

		stage = Stage::GIVE_KILLER_1;
	}

	if (stage == Stage::GIVE_KILLER_1)
	{
		Killer1 = locals.KillerMoves[distanceFromRoot][0];
		stage = Stage::GIVE_KILLER_2;

		if (MoveIsLegal(position, Killer1))
		{
			move = Killer1;
			return true;
		}
	}

	if (stage == Stage::GIVE_KILLER_2)
	{
		Killer2 = locals.KillerMoves[distanceFromRoot][1];
		stage = Stage::GIVE_BAD_LOUD;

		if (MoveIsLegal(position, Killer2))
		{
			move = Killer2;
			return true;
		}
	}

	if (stage == Stage::GIVE_BAD_LOUD)
	{
		if (current != legalMoves.end())
		{
			move = current->move;
			++current;
			return true;
		}
		else
		{
			stage = Stage::GEN_QUIET;
		}
	}

	if (stage == Stage::GEN_QUIET)
	{
		legalMoves.clear();
		QuietMoves(position, legalMoves);
		OrderMoves(legalMoves);
		current = legalMoves.begin();
		stage = Stage::GIVE_QUIET;
	}

	if (stage == Stage::GIVE_QUIET)
	{
		if (current != legalMoves.end())
		{
			move = current->move;
			++current;
			return true;
		}
	}

	return false;
}

void MoveGenerator::AdjustHistory(const Move& move, SearchData& Locals, int depthRemaining) const
{
	Locals.AddHistory(position.GetTurn(), move.GetFrom(), move.GetTo(), depthRemaining * depthRemaining);

	for (auto const& m : legalMoves)
	{
		if (m.move == move) break;
		Locals.AddHistory(position.GetTurn(), m.move.GetFrom(), m.move.GetTo(), -depthRemaining * depthRemaining);
	}
}

void selection_sort(std::vector<ExtendedMove>& v)
{
	for (auto it = v.begin(); it != v.end(); ++it)
	{
		std::iter_swap(it, std::max_element(it, v.end()));
	}
}

constexpr int PieceValues[] = { 91, 532, 568, 715, 1279, 5000,
								91, 532, 568, 715, 1279, 5000 };

static uint64_t AttackersToSq(Position& position, Square sq)
{
	uint64_t pawn_mask = (position.GetPieceBB(PAWN, WHITE) & PawnAttacks[BLACK][sq]);
	pawn_mask |= (position.GetPieceBB(PAWN, BLACK) & PawnAttacks[WHITE][sq]);
	
	uint64_t bishops   = position.GetPieceBB<QUEEN>() | position.GetPieceBB<BISHOP>();
	uint64_t rooks     = position.GetPieceBB<QUEEN>() | position.GetPieceBB<ROOK>();
	uint64_t occ       = position.GetAllPieces();

	return (pawn_mask & position.GetPieceBB<PAWN>())
		| (AttackBB<KNIGHT>(sq) & position.GetPieceBB<KNIGHT>())
		| (AttackBB<KING>(sq) & position.GetPieceBB<KING>())
		| (AttackBB<BISHOP>(sq, occ) & bishops)
		| (AttackBB<ROOK>(sq, occ) & rooks);
}

static uint64_t GetLeastValuableAttacker(Position& position, uint64_t attackers, Pieces& capturing, Players side)
{
	for (int i = 0; i < 6; i++)
	{
		capturing = Piece(PieceTypes(i), side);
		uint64_t pieces = position.GetPieceBB(capturing) & attackers;
		if (pieces)
			return pieces & ~pieces + 1;
	}
	return 0;
}

static int see(Position& position, Move move)
{
	Square from = move.GetFrom();
	Square to   = move.GetTo();

	int scores[32]{ 0 };
	int index = 0;

	auto capturing = position.GetSquare(from);
	auto captured = position.GetSquare(to);
	auto attacker = ColourOfPiece(capturing);

	uint64_t from_set = (1ull << from);
	uint64_t occ = position.GetAllPieces(), bishops = 0, rooks = 0;

	bishops = rooks = position.GetPieceBB<QUEEN>();
	bishops |= position.GetPieceBB<BISHOP>();
	rooks |= position.GetPieceBB<ROOK>();

	uint64_t attack_def = AttackersToSq(position, to);
	scores[index] = PieceValues[captured];

	do
	{
		index++;
		attacker = !attacker;
		scores[index] = PieceValues[capturing] - scores[index - 1];

		if (-scores[index - 1] < 0 && scores[index] < 0)
			break;

		attack_def ^= from_set;
		occ ^= from_set;

		attack_def |= occ & ((bishops & AttackBB<BISHOP>(to, occ)) | (rooks & AttackBB<ROOK>(to, occ)));
		from_set = GetLeastValuableAttacker(position, attack_def, capturing, Players(attacker));
	} while (from_set);
	while (--index)
	{
		scores[index - 1] = -(-scores[index - 1] > scores[index] ? -scores[index - 1] : scores[index]);
	}
	return scores[0];
}

void MoveGenerator::OrderMoves(std::vector<ExtendedMove>& moves)
{
	static constexpr int16_t SCORE_QUEEN_PROMOTION = 30000;
	static constexpr int16_t SCORE_CAPTURE = 20000;
	static constexpr int16_t SCORE_UNDER_PROMOTION = -1;

	for (size_t i = 0; i < moves.size(); i++)
	{
		//Hash move
		if (moves[i].move == TTmove)
		{
			moves.erase(moves.begin() + i);
			i--;
		}

		//Killers
		else if (moves[i].move == Killer1)
		{
			moves.erase(moves.begin() + i);
			i--;
		}

		else if (moves[i].move == Killer2)
		{
			moves.erase(moves.begin() + i);
			i--;
		}

		//Promotions
		else if (moves[i].move.IsPromotion())
		{
			if (moves[i].move.GetFlag() == QUEEN_PROMOTION || moves[i].move.GetFlag() == QUEEN_PROMOTION_CAPTURE)
			{
				moves[i].score = SCORE_QUEEN_PROMOTION;
				moves[i].SEE = PieceValues[QUEEN];
			}
			else
			{
				moves[i].score = SCORE_UNDER_PROMOTION;
			}
		}

		//Captures
		else if (moves[i].move.IsCapture())
		{
			int SEE = 0;

			if (moves[i].move.GetFlag() != EN_PASSANT)
			{
				SEE = see(position, moves[i].move);
			}

			moves[i].score = SCORE_CAPTURE + SEE;
			moves[i].SEE = SEE;
		}

		//Quiet
		else
		{
			moves[i].score = locals.History[position.GetTurn()][moves[i].move.GetFrom()][moves[i].move.GetTo()];
		}
	}

	selection_sort(moves);
}

Move GetHashMove(const Position& position, int depthRemaining, int distanceFromRoot)
{
	TTEntry hash = tTable.GetEntry(position.GetZobristKey(), distanceFromRoot);

	if (CheckEntry(hash, position.GetZobristKey(), depthRemaining))
	{
		tTable.ResetAge(position.GetZobristKey(), position.GetTurnCount(), distanceFromRoot);
		return hash.GetMove();
	}

	return {};
}

Move GetHashMove(const Position& position, int distanceFromRoot)
{
	TTEntry hash = tTable.GetEntry(position.GetZobristKey(), distanceFromRoot);

	if (CheckEntry(hash, position.GetZobristKey()))
	{
		tTable.ResetAge(position.GetZobristKey(), position.GetTurnCount(), distanceFromRoot);
		return hash.GetMove();
	}

	return {};
}