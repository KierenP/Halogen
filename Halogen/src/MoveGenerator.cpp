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
			std::vector<Move> moves;
			LegalMoves(position, moves);	//contains a special function for generating moves when in check which is quicker
			CreateExtendedMoveVector(moves);
		}
		else
		{
			std::vector<Move> moves;
			QuiescenceMoves(position, moves);
			CreateExtendedMoveVector(moves);
		}

		OrderMoves(legalMoves);
		current = legalMoves.begin();

		if (!quiescence && IsInCheck(position))
		{
			stage = Stage::GIVE_QUIET;
		}
		else
		{
			stage = Stage::GIVE_GOOD_LOUD;
		}
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
		std::vector<Move> moves;
		QuietMoves(position, moves);
		CreateExtendedMoveVector(moves);
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
				SEE = seeCapture(position, moves[i].move);
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

void MoveGenerator::CreateExtendedMoveVector(const std::vector<Move>& moves)
{
	legalMoves.clear();
	legalMoves.reserve(moves.size());

	std::copy(moves.begin(), moves.end(), std::back_inserter(legalMoves));
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

int see(Position& position, Square square, Players side)
{
	int value = 0;
	Move capture = GetSmallestAttackerMove(position, square, side);

	if (!capture.IsUninitialized())
	{
		int captureValue = PieceValues[position.GetSquare(capture.GetTo())];

		position.ApplyMoveQuick(capture);
		value = std::max(0, captureValue - see(position, square, !side));	// Do not consider captures if they lose material, therefor max zero 
		position.RevertMoveQuick();
	}

	return value;
}

int seeCapture(Position& position, const Move& move)
{
	assert(move.GetFlag() == CAPTURE);	//Don't seeCapture with promotions or en_passant!

	Players side = position.GetTurn();

	int value = 0;
	int captureValue = PieceValues[position.GetSquare(move.GetTo())];

	position.ApplyMoveQuick(move);
	value = captureValue - see(position, move.GetTo(), !side);
	position.RevertMoveQuick();

	return value;
}