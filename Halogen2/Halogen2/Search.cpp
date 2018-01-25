#include "Search.h"

int NodeCount = 0;

TranspositionTable tTable;
ABnode* SearchToDepth(Position & position, int depth, int alpha, int beta, Move prevBest = Move());
void Maximize(Position & position, int depth, ABnode* parent, int alpha, int beta);
void Minimize(Position & position, int depth, ABnode* parent, int alpha, int beta);
void PrintSearchInfo(Position & position, ABnode& node, unsigned int depth, unsigned int Time, bool isCheckmate);

void NodeReplaceBest(ABnode*& best, ABnode*& node, bool colour);		
void SwapMoves(std::vector<Move>& moves, unsigned int a, unsigned int b);
bool ExtendSearch(Position & position, int depth, Move& move);
bool CheckTransposition(Position & position, int depth, int& alpha, int& beta, ABnode* parent);
bool CheckMateNode(unsigned int size, int depth, bool colour, ABnode* parent);
bool CheckThreeFold(Position & position, ABnode*& node, Move& move, int depth);

bool CheckCutoff(int & alpha, int & beta, ABnode* best, unsigned int cutoff);
ABnode* iterativeDeepening(Position & position, int depth, Move & best, int alpha, int beta, int prevScore);

void OrderMoves(std::vector<Move>& moves, Position & position, int searchDepth)
{
	unsigned int swapIndex = 0;
	Move temp(0, 0, 0);
	int PieceValues[N_PIECES] = { 1, 3, 3, 5, 9, 10, 1, 3, 3, 5, 9, 10 };			//relative values
													
	//move previously cached position to front
	if (tTable.CheckEntry(GenerateZobristKey(position), searchDepth - 1))
	{
		Move bestprev = tTable.GetEntry(GenerateZobristKey(position)).GetMove();
		for (int i = 0; i < moves.size(); i++)
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

	//order in terms of least valuable attacker, most valuable piece
	for (int j = PieceValues[KING] - PieceValues[PAWN]; j >= PieceValues[PAWN] - PieceValues[KING]; j--)
	{
		for (int i = swapIndex; i < moves.size(); i++)
		{
			if (!moves[i].IsCapture())
				continue;

			if (PieceValues[position.GetSquare(moves[i].GetTo())] - PieceValues[position.GetSquare(moves[i].GetFrom())] == j)
			{
				SwapMoves(moves, i, swapIndex);
				swapIndex++;
			}
		}
	}
}

Move SearchPosition(Position & position, int allowedTimeMs, bool printInfo)
{
	tTable.Reformat();

	Move Best;
	int PrevScore = EvaluatePosition(position);		
	SYSTEMTIME before;
	SYSTEMTIME after;
	double prevDepthTime = 1;
	double passedTime = 1;
	unsigned int DepthMultiRatio = 1;
	bool checkmate = false;

	for (int depth = 1; (allowedTimeMs - passedTime > passedTime * DepthMultiRatio * 1.5 || passedTime < allowedTimeMs / 40) && (!checkmate); depth++)
	{
		NodeCount = 0;
		//int alpha = PrevScore - 99999;
		//int beta = PrevScore + 99999;

		GetSystemTime(&before);
		ABnode* ROOT = SearchToDepth(position, depth, -99999, 99999, Best);
		GetSystemTime(&after);

		unsigned int Time = after.wDay * 1000 * 60 * 60 * 24 + after.wHour * 60 * 60 * 1000 + after.wMinute * 60 * 1000 + after.wSecond * 1000 + after.wMilliseconds - before.wDay * 1000 * 60 * 60 * 24 - before.wHour * 60 * 60 * 1000 - before.wMinute * 60 * 1000 - before.wSecond * 1000 - before.wMilliseconds;

		DepthMultiRatio = Time / passedTime;
		prevDepthTime = Time;
		passedTime += Time;

		PrintSearchInfo(position, *ROOT, depth, Time, false);

		if (ROOT->GetCutoff() == CHECK_MATE || ROOT->GetCutoff() == FORCED_MOVE)
			checkmate = true;
	
		std::cout << std::endl;
		Best = ROOT->GetMove();
		PrevScore = ROOT->GetScore();
		delete ROOT;			
	}

	return Best;
}

void PrintSearchInfo(Position & position, ABnode& node, unsigned int depth, unsigned int Time, bool isCheckmate)
{
	std::cout
		<< "info depth " << depth
		<< " seldepth " << node.CountNodeChildren();															//if ABnode tracking is implemented back this will be adjusted

	if (isCheckmate)
		std::cout << " score mate " << (depth + 1) / 2;
	else
		std::cout << " score cp " << (position.GetTurn() * 2 - 1) * node.GetScore();

	std::cout
		<< " time " << Time
		<< " nodes " << NodeCount
		<< " nps " << NodeCount / (Time + 1) * 1000														//+1 to avoid division by zero
		<< " tbhits " << tTable.GetCount()
		<< " pv ";

	node.PrintNodeChildren();
}

void NodeReplaceBest(ABnode*& best, ABnode*& node, bool colour)
{
	if (colour)
	{
		if (node->GetScore() > best->GetScore())
		{
			delete best;
			best = node;
		}
		else
		{
			delete node;
		}
	}
	else
	{
		if (node->GetScore() < best->GetScore())
		{
			delete best;
			best = node;
		}
		else
		{
			delete node;
		}
	}
}

void SwapMoves(std::vector<Move>& moves, unsigned int a, unsigned int b)
{
	Move temp = moves[b];
	moves[b] = moves[a];
	moves[a] = temp;
}

bool ExtendSearch(Position & position, int depth, Move& move)
{
	if (depth > 0)
		return true;

	bool IsReCapture = (move.GetTo() == position.GetCaptureSquare());
	bool IsPromotion = move.IsPromotion();
	bool MeInCheck = IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn());
	bool OtherInCheck = IsInCheck(position, position.GetKing(!position.GetTurn()), !position.GetTurn());

	if (!IsReCapture && !MeInCheck && !OtherInCheck && !IsPromotion)							
		return false;												
	return true;
}

bool CheckTransposition(Position & position, int depth, int & alpha, int & beta, ABnode * parent)
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

bool CheckMateNode(unsigned int size, int depth, bool colour, ABnode * parent)
{
	if (size != 0)
		return false;

	parent->SetChild(CreateCheckmateNode(colour, depth));
	return true;
}

bool CheckThreeFold(Position & position, ABnode*& node, Move& move, int depth)
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

bool CheckCutoff(int & alpha, int & beta, ABnode* best, unsigned int cutoff)
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

ABnode * iterativeDeepening(Position & position, int depth, Move & best, int alpha, int beta, int prevScore)
{
	ABnode * iteration = SearchToDepth(position, depth, alpha, beta, best);

	if (iteration->GetCutoff() == EXACT || iteration->GetCutoff() == CHECK_MATE)
		return iteration;

	delete iteration;

	if (iteration->GetCutoff() == ALPHA_CUTOFF)
	{
		alpha = (alpha - prevScore) * 4 + prevScore;
		std::cout << " alpha cut ";
	}
	if (iteration->GetCutoff() == BETA_CUTOFF)
	{
		beta = (beta - prevScore) * 4 + prevScore;
		std::cout << " beta cut ";
	}

	return iterativeDeepening(position, depth, best, alpha, beta, prevScore);
}

ABnode* SearchToDepth(Position & position, int depth, int alpha, int beta, Move prevBest)
{
	tTable.ResetCount();
	std::vector<Move> moves = GenerateLegalMoves(position);

	//with only one move, we can very quickly just return this move as there is zero benifet to searching the tree below this node
	if (moves.size() == 1)
		return CreateForcedNode(moves[0]);

	OrderMoves(moves, position, depth);
	ABnode* best = CreatePlaceHolderNode(position.GetTurn());

	//Move the previous best move to the front
	for (int i = 0; i < moves.size(); i++)						
	{
		if (moves[i] == prevBest)
		{
			Move temp = moves[i];
			moves[i] = moves[0];
			moves[0] = temp;									
			break;
		}
	}

	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* node = CreateBranchNode(moves[i], depth);

		position.ApplyMove(moves[i]);
		if (position.GetTurn() == BLACK)
			Minimize(position, depth - 1, node, alpha, beta);				
		if (position.GetTurn() == WHITE)
			Maximize(position, depth - 1, node, alpha, beta);
		position.RevertMove(moves[i]);

		//note the move reverting has changed the current turn from before ^
		NodeReplaceBest(best, node, position.GetTurn());
	}

	return best;
}

void Maximize(Position & position, int depth, ABnode* parent, int alpha, int beta)
{
	NodeCount++;
	if (CheckTransposition(position, depth, alpha, beta, parent)) return;
	std::vector<Move> moves = GenerateLegalMoves(position);
	if (CheckMateNode(moves.size(), depth, WHITE, parent)) return;

	OrderMoves(moves, position, depth);
	ABnode* best = CreatePlaceHolderNode(WHITE);
	bool AllQuiet = true;

	for (int i = 0; i < moves.size(); i++)
	{	
		if (!ExtendSearch(position, depth, moves[i]))
			continue;
		AllQuiet = false;

		ABnode* node = CreateBranchNode(moves[i], depth);

		position.ApplyMove(moves[i]);

		if (!CheckThreeFold(position, node, moves[i], depth))
			Minimize(position, depth - 1, node, alpha, beta);

		position.RevertMove(moves[i]);

		NodeReplaceBest(best, node, WHITE);

		if (CheckCutoff(alpha, beta, best, BETA_CUTOFF)) break;
	}

	if (AllQuiet)
	{
		delete best;
		best = CreateLeafNode(position, depth);
	}

	parent->SetChild(best);
	tTable.AddEntry(best->GetMove(), GenerateZobristKey(position), best->GetScore(), depth, best->GetCutoff());
}

void Minimize(Position & position, int depth, ABnode* parent, int alpha, int beta)
{
	NodeCount++;
	if (CheckTransposition(position, depth, alpha, beta, parent)) return;
	std::vector<Move> moves = GenerateLegalMoves(position);
	if (CheckMateNode(moves.size(), depth, BLACK, parent)) return;

	OrderMoves(moves, position, depth);
	ABnode* best = CreatePlaceHolderNode(BLACK);
	bool AllQuiet = true;

	for (int i = 0; i < moves.size(); i++)
	{
		if (!ExtendSearch(position, depth, moves[i]))
			continue;
		AllQuiet = false;

		ABnode* node = CreateBranchNode(moves[i], depth);
		position.ApplyMove(moves[i]);

		if (!CheckThreeFold(position, node, moves[i], depth))
			Maximize(position, depth - 1, node, alpha, beta);

		position.RevertMove(moves[i]);
		NodeReplaceBest(best, node, BLACK);
		
		if (CheckCutoff(alpha, beta, best, ALPHA_CUTOFF)) break;
	}

	if (AllQuiet)
	{
		delete best;
		best = CreateLeafNode(position, depth);
	}

	parent->SetChild(best);
	tTable.AddEntry(best->GetMove(), GenerateZobristKey(position), best->GetScore(), depth, best->GetCutoff());
}
