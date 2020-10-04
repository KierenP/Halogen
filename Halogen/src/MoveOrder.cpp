#include "MoveOrder.h"

void OrderMoves(std::vector<Move>& moves, Position& position, int distanceFromRoot, const std::vector<Killer>& KillerMoves, unsigned int(&HistoryMatrix)[N_PLAYERS][N_SQUARES][N_SQUARES], Move TTmove);
void SortMovesByScore(std::vector<Move>& moves, std::vector<int>& orderScores);

MoveGenerator::MoveGenerator()
{
	state = Stage::TT_MOVE;
	currentIndex = -1;
}

MoveGenerator::~MoveGenerator()
{
}

bool MoveGenerator::GetNext(Move& move, Position& position, int distanceFromRoot, const std::vector<Killer>& KillerMoves, unsigned int(&HistoryMatrix)[N_PLAYERS][N_SQUARES][N_SQUARES])
{
	std::vector<Move> moves;

	switch (state)
	{
	case Stage::TT_MOVE:
		move = GetHashMove(position, distanceFromRoot); 
		if (!move.IsUninitialized())
		{	
			state = Stage::CAPTURES;
			TTmove = move;
			return true;
		}
		//Fall through

	case Stage::CAPTURES:
		
		if (currentIndex == -1) 
		{
			QuiescenceMoves(position, loudMoves);
			OrderMoves(loudMoves, position, distanceFromRoot, KillerMoves, HistoryMatrix, TTmove);
			currentIndex++;
		}

		if (currentIndex < loudMoves.size())
		{
			move = loudMoves[currentIndex];
			currentIndex++;
			return true;
		}
		else 
		{
			state = Stage::QUIET_MOVES;
			currentIndex = -1;
		}
		//Fall through

	case Stage::QUIET_MOVES:
		if (currentIndex == -1) 
		{
			QuietMoves(position, quietMoves);
			OrderMoves(quietMoves, position, distanceFromRoot, KillerMoves, HistoryMatrix, TTmove);
			currentIndex++;
		}

		if (currentIndex < quietMoves.size()) 
		{
			move = quietMoves[currentIndex];
			currentIndex++;
			return true;
		}
	}

	return false;
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

void OrderMoves(std::vector<Move>& moves, Position& position, int distanceFromRoot, const std::vector<Killer>& KillerMoves, unsigned int(&HistoryMatrix)[N_PLAYERS][N_SQUARES][N_SQUARES], Move TTmove)
{
	/*
	Previously this function would order all the moves at once. Now it only orderes the section we are generating (for example just the loud moves).
	*/

	for (size_t i = 0; i < moves.size(); i++)
	{
		//Hash move
		if (moves[i] == TTmove)
		{
			moves.erase(moves.begin() + i);
			i--;
		}

		//Promotions
		else if (moves[i].IsPromotion())
		{
			if (moves[i].GetFlag() == QUEEN_PROMOTION || moves[i].GetFlag() == QUEEN_PROMOTION_CAPTURE)
			{
				moves[i].orderScore = static_cast<int>(MoveScore::PROMOTION);
			}
			else
			{
				moves[i].orderScore = static_cast<int>(MoveScore::UNDERPROMOTION);
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
				moves[i].orderScore = static_cast<int>(MoveScore::GOOD_CAPTURE) + SEE;
			}

			if (SEE < 0)
			{
				moves[i].orderScore = static_cast<int>(MoveScore::BAD_CAPTURE) + SEE;
			}
		}

		//Killers
		else if (moves[i] == KillerMoves.at(distanceFromRoot).move[0])
		{
			moves[i].orderScore = static_cast<int>(MoveScore::KILLER_1);
		}

		else if (moves[i] == KillerMoves.at(distanceFromRoot).move[1])
		{
			moves[i].orderScore = static_cast<int>(MoveScore::KILLER_2);
		}

		else 
		{
			//Quiet
			moves[i].orderScore = HistoryMatrix[position.GetTurn()][moves[i].GetFrom()][moves[i].GetTo()];

			if (moves[i].orderScore > static_cast<int>(MoveScore::QUIET_MAXIMUM))
			{
				moves[i].orderScore = static_cast<int>(MoveScore::QUIET_MAXIMUM);
			}
		}
	}

	std::sort(moves.begin(), moves.end(), [](const Move& lhs, const Move& rhs)
		{
			return lhs.orderScore > rhs.orderScore;
		});
}

void SortMovesByScore(std::vector<Move>& moves, std::vector<int>& orderScores)
{
	//selection sort
	for (size_t i = 0; static_cast<int>(i)< static_cast<int>(moves.size()) - 1; i++)
	{
		size_t max = i;

		for (size_t j = i + 1; j < moves.size(); j++)
		{
			if (orderScores[j] > orderScores[max])
			{
				max = j;
			}
		}

		if (max != i)
		{
			std::swap(moves[i], moves[max]);
			std::swap(orderScores[i], orderScores[max]);
		}
	}
}

int see(Position& position, int square, bool side)
{
	int value = 0;
	Move capture = GetSmallestAttackerMove(position, square, side);

	if (!capture.IsUninitialized())
	{
		int captureValue = PieceValues(position.GetSquare(capture.GetTo()));

		position.ApplySEECapture(capture);
		value = std::max(0, captureValue - see(position, square, !side));	// Do not consider captures if they lose material, therefor max zero 
		position.RevertSEECapture();
	}

	return value;
}

int seeCapture(Position& position, const Move& move)
{
	assert(move.GetFlag() == CAPTURE);	//Don't seeCapture with promotions or en_passant!

	bool side = position.GetTurn();

	int value = 0;
	int captureValue = PieceValues(position.GetSquare(move.GetTo()));

	position.ApplySEECapture(move);
	value = captureValue - see(position, move.GetTo(), !side);
	position.RevertSEECapture();

	return value;
}
