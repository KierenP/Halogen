#include "MoveGenerator.h"

MoveGenerator::MoveGenerator(Position& Position, int DistanceFromRoot, const SearchData& Locals, bool Quiescence) :
	position(Position), distanceFromRoot(DistanceFromRoot), locals(Locals), quiescence(Quiescence)
{
	stage = Stage::TT_MOVE;
}

bool MoveGenerator::Next(Move& move)
{
	if (stage == Stage::TT_MOVE)
	{
		move = GetHashMove(position, distanceFromRoot);
		stage = Stage::GEN_OTHERS;
		
		if (!move.IsUninitialized())
			return true;
	}

	if (stage == Stage::GEN_OTHERS)
	{
		if (quiescence)
			QuiescenceMoves(position, legalMoves);
		else
			LegalMoves(position, legalMoves);

		OrderMoves(legalMoves);
		current = legalMoves.begin();
		stage = Stage::GIVE_OTHERS;
	}

	if (stage == Stage::GIVE_OTHERS)
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

	Move TTmove = GetHashMove(position, distanceFromRoot);

	for (size_t i = 0; i < moves.size(); i++)
	{
		//Hash move
		if (moves[i] == TTmove)
		{
			moves.erase(moves.begin() + i);
			i--;
			continue; 
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

		//Killers
		else if (moves[i] == locals.KillerMoves[distanceFromRoot][0])
		{
			moves[i].orderScore = 7500000;
		}

		else if (moves[i] == locals.KillerMoves[distanceFromRoot][1])
		{
			moves[i].orderScore = 6500000;
		}

		//Quiet
		else
		{
			moves[i].orderScore = std::min(1000000U, locals.HistoryMatrix[position.GetTurn()][moves[i].GetFrom()][moves[i].GetTo()]);
		}
	}

	std::stable_sort(moves.begin(), moves.end(), [](const Move& a, const Move& b)
		{
			return a.orderScore > b.orderScore;
		});
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