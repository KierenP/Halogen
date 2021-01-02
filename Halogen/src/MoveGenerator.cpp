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
		}
		else
		{
			QuiescenceMoves(position, legalMoves);
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
		if (current != legalMoves.end() && current->orderScore >= 8000000)
		{
			move = *current;
			++current;
			return true;
		}
		else
		{
			if (quiescence)
				return false;

			stage = Stage::GIVE_KILLER_1;
		}
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
			move = *current;
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
			move = *current;
			++current;
			return true;
		}
	}

	return false;
}

void selection_sort(std::vector<Move>& v)
{
	for (auto it = v.begin(); it != v.end(); ++it)
	{
		std::iter_swap(it, std::max_element(it, v.end()));
	}
}

void MoveGenerator::OrderMoves(std::vector<Move>& moves)
{
	/*
	We want to order the moves such that the best moves are more likely to be further towards the front.

	The order is as follows:

	1. Hash move												= 10m
	2. Queen Promotions											= 9m
	3. Winning captures											= +8m
	4. Killer moves												= ~7m
	5. Losing captures											= -6m
	6. Quiet moves (further sorted by history matrix values)	= 0-1m
	7. Underpromotions											= -1

	Note that typically the maximum value of the history matrix does not exceed 1,000,000 after a minute
	and as such we choose 1m to be the maximum allowed value
	*/

	for (size_t i = 0; i < moves.size(); i++)
	{
		//Hash move
		if (moves[i] == TTmove)
		{
			moves.erase(moves.begin() + i);
			i--;
		}

		//Killers
		else if (moves[i] == Killer1)
		{
			moves.erase(moves.begin() + i);
			i--;
		}

		else if (moves[i] == Killer2)
		{
			moves.erase(moves.begin() + i);
			i--;
		}

		//Promotions
		else if (moves[i].IsPromotion())
		{
			if (moves[i].GetFlag() == QUEEN_PROMOTION || moves[i].GetFlag() == QUEEN_PROMOTION_CAPTURE)
			{
				moves[i].orderScore = 9000000;
			}
			else
			{
				moves[i].orderScore = -1;
			}
		}

		//Captures
		else if (moves[i].IsCapture())
		{
			int SEE = 0;

			if (moves[i].GetFlag() != EN_PASSANT)
			{
				SEE = seeCapture(position, moves[i]);
			}

			if (SEE >= 0)
			{
				moves[i].orderScore = 8000000 + SEE;
			}

			if (SEE < 0)
			{
				moves[i].orderScore = 6000000 + SEE;
			}
		}

		//Quiet
		else
		{
			moves[i].orderScore = std::min(1000000U, locals.HistoryMatrix[position.GetTurn()][moves[i].GetFrom()][moves[i].GetTo()]);
		}
	}

	selection_sort(moves);
}

Move GetHashMove(const Position& position, int depthRemaining, int distanceFromRoot)
{
	TTEntry hash = tTable.GetEntry(position.GetZobristKey());

	if (CheckEntry(hash, position.GetZobristKey(), depthRemaining))
	{
		tTable.SetNonAncient(position.GetZobristKey(), position.GetTurnCount(), distanceFromRoot);
		return hash.GetMove();
	}

	return {};
}

Move GetHashMove(const Position& position, int distanceFromRoot)
{
	TTEntry hash = tTable.GetEntry(position.GetZobristKey());

	if (CheckEntry(hash, position.GetZobristKey()))
	{
		tTable.SetNonAncient(position.GetZobristKey(), position.GetTurnCount(), distanceFromRoot);
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
		int captureValue = PieceValues(position.GetSquare(capture.GetTo()));

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
	int captureValue = PieceValues(position.GetSquare(move.GetTo()));

	position.ApplyMoveQuick(move);
	value = captureValue - see(position, move.GetTo(), !side);
	position.RevertMoveQuick();

	return value;
}