#include "Search.h"

enum SearchLevels
{
	MINMAX,
	ALPHABETA,
	LATE_MOVE_REDUCTION,
	TERMINATE,
};

TranspositionTable tTable;
clock_t SearchBegin;
clock_t CurrentTime;

int AllowedSearchTime = 0;	//in ms. 
bool AbortSearch = false;	//Periodically check CurrentTime - SearchBegin and if greater than some value then set this to true.

ABnode* SearchToDepth(Position & position, int depth, int alpha, int beta);
std::vector<ABnode*> SearchDebug(Position& position, int depth, int alpha, int beta);		//Return the best 4 moves in order from this position
void PrintSearchInfo(Position & position, ABnode& node, unsigned int depth, double Time, bool isCheckmate);
void SwapMoves(std::vector<Move>& moves, unsigned int a, unsigned int b);
SearchLevels CalculateSearchType(Position& position, Move& move, int depth, bool check, int index);
bool CheckForCutoff(int & alpha, int & beta, ABnode* best, unsigned int cutoff);
bool CheckForTransposition(Position & position, int depth, int& alpha, int& beta, ABnode* parent);
bool CheckForCheckmate(Position & position, unsigned int size, int depth, bool colour, ABnode* parent);
bool CheckForDraw(Position & position, ABnode*& node, Move& move, int depth);
void SetBest(ABnode*& best, ABnode*& node, bool colour);
bool InitializeSearchVariables(Position & position, std::vector<Move>& moves, int depth, int & alpha, int & beta, ABnode* parent, bool allowNull, bool colour);
void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth);
void MinMax(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search);
void CheckTime();
bool LateMoveReduction(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search);

void OrderMoves(std::vector<Move>& moves, Position & position, int searchDepth)
{
	/*
	We want to order the moves such that the best moves are more likely to be further towards the front.
	*/

	unsigned int swapIndex = 0;														
	int PieceValues[N_PIECES] = { 1, 3, 3, 5, 9, 100, 1, 3, 3, 5, 9, 100 };			//relative values
													
	//move previously cached position to front
	if (tTable.CheckEntry(GenerateZobristKey(position), searchDepth - 1))
	{
		Move bestprev = tTable.GetEntry(GenerateZobristKey(position)).GetMove();
		for (int i = swapIndex; i < moves.size(); i++)
		{
			if (bestprev == moves[i])
			{
				SwapMoves(moves, i, swapIndex);
				swapIndex++;
				break;
			}
		}
	}

	//The move promotions to front
	for (int i = swapIndex; i < moves.size(); i++)
	{
		if (moves[i].IsPromotion())
		{
			SwapMoves(moves, i, swapIndex);
			swapIndex++;
		}
	}

	std::vector<int> CaptureDifference;
	std::vector<unsigned int> CaptureIndexes;

	CaptureDifference.reserve(10);
	CaptureIndexes.reserve(10);

	for (int i = swapIndex; i < moves.size(); i++)
	{
		if (!moves[i].IsCapture())
			continue;

		CaptureDifference.push_back(PieceValues[position.GetSquare(moves[i].GetTo())] - PieceValues[position.GetSquare(moves[i].GetFrom())]);
		CaptureIndexes.push_back(i);
	}

	//Order the captures from the biggest difference to the lowest  (eg queen taken by pawn would be closer to front than rook takes rook)
	for (int j = PieceValues[KING] - PieceValues[PAWN]; j >= PieceValues[PAWN] - PieceValues[KING]; j--)
	{
		for (int i = 0; i < CaptureDifference.size(); i++)
		{
			if (CaptureDifference[i] == j)
			{
				SwapMoves(moves, CaptureIndexes[i], swapIndex);

				for (int k = 0; k < CaptureIndexes.size(); k++)
				{
					if (CaptureIndexes[k] == swapIndex)
						CaptureIndexes[k] = i;
				}
				CaptureIndexes[i] = swapIndex;

				swapIndex++;
			}
		}
	}
}

Move SearchPosition(Position & position, int allowedTimeMs, bool printInfo)
{
	SearchBegin = clock();
	CurrentTime = clock();
	CheckTime();

	AbortSearch = false;
	AllowedSearchTime = allowedTimeMs;
	tTable.SetAllAncient();

	std::vector<Move> moves = GenerateLegalMoves(position);

	if (moves.size() == 1)	//If there is only one legal move, then we just play that move immediantly
		return moves[0];

	Move Best;
	int score = EvaluatePosition(position);
	bool endSearch = false;

	//abort if I have used up more than half the time, if we have used up less than half it will try and search another ply.
	for (int depth = 1; !AbortSearch && !endSearch && ((double(CurrentTime) - double(SearchBegin)) / CLOCKS_PER_SEC * 1000 < AllowedSearchTime / 2); depth++)
	{
		NodeCount = 0;

		clock_t before = std::clock();
		//ABnode* ROOT = SearchToDepth(position, depth, LowINF, HighINF);
		ABnode* ROOT = SearchToDepth(position, depth, LowINF, HighINF);
		clock_t after = std::clock();

		if (AbortSearch)
		{	//stick with what we previously found to be best if we had to abort.
			delete ROOT;
			break;
		}

		double elapsed_ms = (double(after) - double(before)) / CLOCKS_PER_SEC * 1000;
		PrintSearchInfo(position, *ROOT, depth, elapsed_ms, false);
		std::cout << std::endl;

		if (ROOT->GetCutoff() == CHECK_MATE)	//no need to search deeper if this is the case.
			endSearch = true;
	
		Best = ROOT->GetMove();
		score = ROOT->GetScore();
		delete ROOT;	
	}

	return Best;
}

std::vector<Move> SearchBenchmark(Position& position, int allowedTimeMs, bool printInfo)
{
	AbortSearch = false;
	AllowedSearchTime = allowedTimeMs;
	SearchBegin = clock();

	tTable.SetAllAncient();

	std::vector<Move> moves = GenerateLegalMoves(position);
	std::vector<Move> best;

	Move Best[4];

	double passedTime = 1;
	unsigned int DepthMultiRatio = 1;
	bool endSearch = false;

	for (int depth = 1; !AbortSearch && !endSearch; depth++)
	{
		NodeCount = 0;

		clock_t before = std::clock();
		std::vector<ABnode*> RankedMoves = SearchDebug(position, depth, LowINF, HighINF);
		clock_t after = std::clock();

		double elapsed_ms = (double(after) - double(before)) / CLOCKS_PER_SEC * 1000;

		if (RankedMoves[0]->GetCutoff() == CHECK_MATE)
			endSearch = true;

		best.clear();
		best.push_back(RankedMoves[0]->GetMove());
		best.push_back(RankedMoves[1]->GetMove());
		best.push_back(RankedMoves[2]->GetMove());
		best.push_back(RankedMoves[3]->GetMove());

		for (int i = 0; i < RankedMoves.size(); i++)
		{
			//PrintSearchInfo(position, *RankedMoves[i], depth, elapsed_ms, false);
			//std::cout << " " << RankedMoves[i]->GetCutoff();
			//std::cout << std::endl;
			delete RankedMoves[i];
		}
	}

	return best;
}

void PrintSearchInfo(Position & position, ABnode& node, unsigned int depth, double Time, bool isCheckmate)
{
	//TODO: Figure out wtf seldepth was meant to mean and add on comments describing what each thing means

	std::cout
		<< "info depth " << depth																				//the depth of search
		<< " seldepth " << node.CountNodeChildren();															//the selective depth (for example searching further for checks and captures)

	if (isCheckmate)
		std::cout << " score mate " << (depth + 1) / 2;															//TODO: should use negative value if the engine is being mated
	else
		std::cout << " score cp " << (position.GetTurn() * 2 - 1) * node.GetScore();							//The score in hundreths of a pawn (a 1 pawn advantage is +100)

	std::cout
		<< " time " << Time																						//Time in ms
		<< " nodes " << NodeCount
		<< " nps " << NodeCount / (Time) * 1000														
		<< " tbhits " << tTable.GetHitCount()
		<< " hashfull " << unsigned int(float(tTable.GetCapacity()) / TTSize * 1000)							//thousondths full
		<< " pv ";																								//the current best line found

	node.PrintNodeChildren();
}

void SetBest(ABnode*& best, ABnode*& node, bool colour)
{
	if ((colour == WHITE && node->GetScore() > best->GetScore()) || (colour == BLACK && node->GetScore() < best->GetScore()))
	{
		delete best;
		best = node;
	}
	else
		delete node;
}

void SwapMoves(std::vector<Move>& moves, unsigned int a, unsigned int b)
{
	Move temp = moves[b];
	moves[b] = moves[a];
	moves[a] = temp;
}

SearchLevels CalculateSearchType(Position& position, Move& move, int depth, bool check, int index)
{
	if (depth <= -2)
		return TERMINATE;

	if (check)
		return ALPHABETA;

	if (move.IsPromotion())
		return ALPHABETA;

	if (move.IsCapture() && !(position.GetSquare(move.GetTo()) == WHITE_PAWN || position.GetSquare(move.GetTo()) == BLACK_PAWN))	//capture of non-pawn
		return ALPHABETA;

	if (index >= 4 && depth >= 3)
	{
		return LATE_MOVE_REDUCTION;
	}

	if (depth > 1)
		return ALPHABETA;

	return TERMINATE;
}

bool CheckForTransposition(Position & position, int depth, int & alpha, int & beta, ABnode * parent)
{
	if (tTable.CheckEntry(GenerateZobristKey(position), depth))
	{
		tTable.AddHit();
		TTEntry ttEntry = tTable.GetEntry(GenerateZobristKey(position));
		if (ttEntry.GetCutoff() == EXACT || ttEntry.GetCutoff() == CHECK_MATE)
		{
			parent->SetChild(new ABnode(Move(), depth, ttEntry.GetCutoff(), ttEntry.GetScore()));
			return true;
		}
		if (ttEntry.GetCutoff() == ALPHA_CUTOFF)
			alpha = max(alpha, ttEntry.GetScore());
		if (ttEntry.GetCutoff() == BETA_CUTOFF)
			beta = min(beta, ttEntry.GetScore());
	}

	return false;
}

bool CheckForCheckmate(Position & position, unsigned int size, int depth, bool colour, ABnode * parent)
{
	if (size != 0)
		return false;

	if (!IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()))
		parent->SetChild(CreateDrawNode(Move(), depth));	//Stalemate
	else
		parent->SetChild(CreateCheckmateNode(colour, depth));

	return true;
}

bool CheckForDraw(Position & position, ABnode*& node, Move& move, int depth)
{
	int rep = 0;
	uint64_t current = PreviousKeys[PreviousKeys.size() - 1];

	for (int i = 0; i < PreviousKeys.size(); i++)
	{
		if (PreviousKeys[i] == current)
			rep++;
	}

	if (rep >= 3)
	{
		delete node;
		node = CreateDrawNode(move, depth);
		return true;
	}
	return false;
}

bool CheckForCutoff(int & alpha, int & beta, ABnode* best, unsigned int cutoff)
{
	if (cutoff == BETA_CUTOFF)
	{
		alpha = max(alpha, best->GetScore());
		if (alpha >= beta)
		{
			best->SetCutoff(BETA_CUTOFF);
			return true;
		}
	}
	if (cutoff == ALPHA_CUTOFF)
	{
		beta = min(beta, best->GetScore());
		if (beta <= alpha)
		{
			best->SetCutoff(ALPHA_CUTOFF);
			return true;
		}
	}

	return false;
}

bool InitializeSearchVariables(Position & position, std::vector<Move>& moves, int depth, int & alpha, int & beta, ABnode* parent, bool allowNull, bool colour)
{
	if (CheckForTransposition(position, depth, alpha, beta, parent)) return true;

	moves = GenerateLegalMoves(position);
	if (CheckForCheckmate(position, moves.size(), depth, colour, parent)) return true;

	OrderMoves(moves, position, depth);
	return false;
}

ABnode* SearchToDepth(Position & position, int depth, int alpha, int beta)
{
	tTable.ResetHitCount();

	std::vector<Move> moves = GenerateLegalMoves(position);
	ABnode* best = CreatePlaceHolderNode(position.GetTurn(), depth);

	OrderMoves(moves, position, depth);

	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* node = CreateBranchNode(moves[i], depth);

		position.ApplyMove(moves[i]);
		MinMax(position, depth - 1, node, alpha, beta, true, ALPHABETA);
		position.RevertMove(moves[i]);

		if (AbortSearch)	//go with what was previously found to be the best move so far, as he had to stop the search early.
			break;

		SetBest(best, node, position.GetTurn());
	}

	tTable.AddEntry(TTEntry(best->GetMove(), GenerateZobristKey(position), best->GetScore(), depth, best->GetCutoff()));
	return best;
}

std::vector<ABnode*> SearchDebug(Position& position, int depth, int alpha, int beta)
{
	tTable.ResetHitCount();

	std::vector<Move> moves = GenerateLegalMoves(position);
	std::vector<ABnode*> MoveNodes;

	OrderMoves(moves, position, depth);

	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* node = CreateBranchNode(moves[i], depth);
		position.ApplyMove(moves[i]);
		MinMax(position, depth - 1, node, alpha, beta, true, ALPHABETA);
		position.RevertMove(moves[i]);
		MoveNodes.push_back(node);
	}

	//Quick and dirty swap sort
	bool swapped = false;
	do
	{
		swapped = false;
		for (int i = 0; i < MoveNodes.size() - 1; i++)
		{
			if ((position.GetTurn() == WHITE && MoveNodes[i]->GetScore() < MoveNodes[i + 1]->GetScore()) || (position.GetTurn() == BLACK && MoveNodes[i]->GetScore() > MoveNodes[i + 1]->GetScore()))
			{
				std::iter_swap(MoveNodes.begin() + i, MoveNodes.begin() + i + 1);
				swapped = true;
			}
		}
	} while (swapped);

	tTable.AddEntry(TTEntry(MoveNodes[0]->GetMove(), GenerateZobristKey(position), MoveNodes[0]->GetScore(), depth, MoveNodes[0]->GetCutoff()));
	return MoveNodes;
}

void MinMax(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search)
{
	CurrentTime = clock();
	CheckTime();

	std::vector<Move> moves;
	if (InitializeSearchVariables(position, moves, depth, alpha, beta, parent, allowNull, position.GetTurn())) return;

	ABnode* best = CreatePlaceHolderNode(position.GetTurn(), depth);
	bool InCheck = IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn());

	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* node = CreateBranchNode(moves[i], depth);
		SearchLevels Newsearch = CalculateSearchType(position, moves[i], depth, InCheck, i);

		position.ApplyMove(moves[i]);
		if (!CheckForDraw(position, node, moves[i], depth))
		{
			bool Cut = false;

			if (Newsearch == LATE_MOVE_REDUCTION)
				if (LateMoveReduction(position, depth, node, alpha, beta, true, Newsearch))
					Cut = true;

			if (!Cut)
			{
				//If we are aborting, it's automatic stop search. If we are at a TERMINATE then keep going if the move gave check; There won't be many legal moves to search anyways
				if ((Newsearch == TERMINATE && !IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn())) || AbortSearch)
				{
					delete node;
					node = new ABnode(moves[i], depth, EXACT, EvaluatePosition(position));
				}
				else
				{
					MinMax(position, depth - 1, node, alpha, beta, true, Newsearch);
				}
			}
		}
		position.RevertMove(moves[i]);

		SetBest(best, node, position.GetTurn());
		if (CheckForCutoff(alpha, beta, best, position.GetTurn() ? BETA_CUTOFF : ALPHA_CUTOFF)) break;
	}

	parent->SetChild(best);
	tTable.AddEntry(TTEntry(best->GetMove(), GenerateZobristKey(position), best->GetScore(), depth, best->GetCutoff()));
}

void CheckTime()
{
	if ((double(CurrentTime) - double(SearchBegin)) / CLOCKS_PER_SEC * 1000 > AllowedSearchTime)
		AbortSearch = true;
}

bool LateMoveReduction(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search)
{
	if (position.GetTurn() == BLACK)
	{ //we just had a white move played and it is now blacks turn to move
		MinMax(position, depth - 2, parent, alpha, alpha + 1, true, search);	

		if (parent->GetCutoff() == ALPHA_CUTOFF)
			return true;
		return false;
	}

	if (position.GetTurn() == WHITE)
	{ //we just had a white move played and it is now blacks turn to move
		MinMax(position, depth - 2, parent, beta - 1, beta, true, search);		

		if (parent->GetCutoff() == BETA_CUTOFF)
			return true;
		return false;
	}

}
