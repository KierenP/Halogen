#include "Search.h"

enum SearchLevels
{
	MINMAX,
	ALPHABETA,
	FUTILE_BRANCH,
	QUIETESSENCE,
	TERMINATE,
};

TranspositionTable tTable;
ABnode* SearchToDepth(Position & position, int depth, int alpha, int beta, Move prevBest = Move());
void Maximize(Position & position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search);
void Minimize(Position & position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search);
void PrintSearchInfo(Position & position, ABnode& node, unsigned int depth, unsigned int Time, bool isCheckmate);

void SwapMoves(std::vector<Move>& moves, unsigned int a, unsigned int b);

SearchLevels CalculateSearchType(Position & position, Move& move, int depth, int alpha, int beta, Players colour, SearchLevels search);

bool CheckForCutoff(int & alpha, int & beta, ABnode* best, unsigned int cutoff);
bool CheckForTransposition(Position & position, int depth, int& alpha, int& beta, ABnode* parent);
bool CheckForCheckmate(Position & position, unsigned int size, int depth, bool colour, ABnode* parent);
bool CheckForDraw(Position & position, ABnode*& node, Move& move, int depth);
bool NullMovePrune(Position & position, ABnode* parent, bool colour, int alpha, int beta, int depth);
bool FutilityPrune(Position & position, Move & move, int alpha, int beta, int depth, bool colour, int staticEval, bool isfutile);

void SetBest(ABnode*& best, ABnode*& node, bool colour);
bool InitializeSearchVariables(Position & position, std::vector<Move>& moves, int depth, int & alpha, int & beta, ABnode* parent, bool allowNull, bool colour);

void OrderMoves(std::vector<Move>& moves, Position & position, int searchDepth)
{
	unsigned int swapIndex = 0;
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
	/*for (int j = PieceValues[KING] - PieceValues[PAWN]; j >= PieceValues[PAWN] - PieceValues[KING]; j--)
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
	}*/

	//This is terrible, dangerous code. But it works, and it's fast.

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
	tTable.Reformat();

	Move Best;
	int PrevScore = EvaluatePosition(position);		
	SYSTEMTIME before;
	SYSTEMTIME after;
	double prevDepthTime = 1;
	double passedTime = 1;
	unsigned int DepthMultiRatio = 1;
	bool endSearch = false;

	for (int depth = 1; (allowedTimeMs - passedTime > passedTime * DepthMultiRatio * 1.5 || passedTime < allowedTimeMs / 20) && (!endSearch); depth++)
	{
		NodeCount = 0;

		GetSystemTime(&before);
		ABnode* ROOT = SearchToDepth(position, depth, LowINF, HighINF, Best);
		GetSystemTime(&after);

		unsigned int Time = after.wDay * 1000 * 60 * 60 * 24 + after.wHour * 60 * 60 * 1000 + after.wMinute * 60 * 1000 + after.wSecond * 1000 + after.wMilliseconds - before.wDay * 1000 * 60 * 60 * 24 - before.wHour * 60 * 60 * 1000 - before.wMinute * 60 * 1000 - before.wSecond * 1000 - before.wMilliseconds;

		DepthMultiRatio = Time / passedTime;
		prevDepthTime = Time;
		passedTime += Time;

		PrintSearchInfo(position, *ROOT, depth, Time, false);

		if (ROOT->GetCutoff() == CHECK_MATE || ROOT->GetCutoff() == FORCED_MOVE)
			endSearch = true;
	
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
		<< "info depth " << depth + 2
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
		<< " hashfull " << unsigned int(float(tTable.GetCapacity()) / TTSize * 1000)
		<< " pv ";

	node.PrintNodeChildren();
}

void SetBest(ABnode*& best, ABnode*& node, bool colour)
{
	if (colour == WHITE)
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

SearchLevels CalculateSearchType(Position & position, Move& move, int depth, int alpha, int beta, Players colour, SearchLevels search)
{
	/*
	SUMMARY OF BELOW LOGIC:
	-If we are in a minmax search, keep doing minmax until depth <= 0, then terminate.
	-If we are in a final extension, we only look further one more level again for RECAPTURES, PROMOTIONS and CHECKS
	-If we are in a futile branch
		-and depth is now <= 0, we switch to a extend one branch just like we would if we reached the end of alphabeta
		-look another level further for ALL CAPTURES, PROMOTIONS and CHECKS
		-if none exist, terminate
	-In alpha beta:
		-for depth > 3, keep doing alphabeta
		-for 0 < depth <= 3, we might reach a futile branch, so check for that, if its not futile or either is in check, continue alphabeta
		-for depth <= 0, extend one more
	
	*/

	if (search == MINMAX)
	{
		if (depth > 0)
			return MINMAX;
		return TERMINATE;
	}

	if (search == QUIETESSENCE)
	{
		if (move.IsCapture() && (move.GetTo() == PreviousParamiters[PreviousParamiters.size() - 2].GetCaptureSquare())) return QUIETESSENCE;		//recapture
		if (move.IsPromotion())																							return QUIETESSENCE;		//promotion
		if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()))								return QUIETESSENCE;		//me in check
		if (IsInCheck(position, position.GetKing(!position.GetTurn()), !position.GetTurn()))							return QUIETESSENCE;		//other in check

		return TERMINATE;
	}

	if (search == FUTILE_BRANCH)
	{
		if (depth <= 0) return QUIETESSENCE;

		if (move.IsCapture())																							return FUTILE_BRANCH;	//any capture
		if (move.IsPromotion())																							return FUTILE_BRANCH;	//promotion
		if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()))								return FUTILE_BRANCH;	//me in check
		if (IsInCheck(position, position.GetKing(!position.GetTurn()), !position.GetTurn()))							return FUTILE_BRANCH;	//other in check

		return QUIETESSENCE;																														//If none of the above
	}

	if (search == ALPHABETA)
	{
		if (depth > 3)
			return ALPHABETA;

		if (depth > 0)
		{
			if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()))							return ALPHABETA;		//me in check
			if (IsInCheck(position, position.GetKing(!position.GetTurn()), !position.GetTurn()))						return ALPHABETA;		//other in check

			//Futility pruning
			if (colour == WHITE)									//we are trying to raise alpha (find a better move than any searched yet)
			{
				if (EvaluatePosition(position) + 100 <= alpha)		//if this is true, it would be futile to analyze all the moves in this line as it is bad compared to the best line found so far, so from now on we will just look at all captures promotions and checks
					return FUTILE_BRANCH;
			}
			else
			{
				if (EvaluatePosition(position) - 100 >= beta)
					return FUTILE_BRANCH;
			}

			return ALPHABETA;
		}

		return QUIETESSENCE;
	}

	return TERMINATE;

	/*int RelativePieceValues[N_PIECES] = { 1, 3, 3, 5, 9, 10, 1, 3, 3, 5, 9, 10 };

	if (futileBranch && depth > 0)
	{
		if (move.IsCapture()) return true;	
		if (move.IsPromotion()) return true;																//promotion
		if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn())) return true;		//me in check
		if (IsInCheck(position, position.GetKing(!position.GetTurn()), !position.GetTurn())) return true;	//other in check

		return false;
	}

	if (depth > 0)
		return true;

	//If I am capturing something more valuable than myself 
	//if (move.IsCapture() && (RelativePieceValues[position.GetSquare(move.GetTo())] < RelativePieceValues[position.GetSquare(move.GetFrom())])) return true;
	if (move.IsPromotion()) return true;																	//promotion
	if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn())) return true;			//me in check
	if (IsInCheck(position, position.GetKing(!position.GetTurn()), !position.GetTurn())) return true;		//other in check
											
	return false;*/
}

bool CheckForTransposition(Position & position, int depth, int & alpha, int & beta, ABnode * parent)
{
	if (tTable.CheckEntry(PreviousKeys[PreviousKeys.size() - 1], depth))
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
		parent->SetChild(CreateDrawNode(Move(), depth));
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

bool NullMovePrune(Position & position, ABnode* parent, bool colour, int alpha, int beta, int depth)
{
	//alpha += 100;
	//beta -= 100;

	if (depth <= 4)
		return false;

	if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn())) return false;			
	if (IsInCheck(position, position.GetKing(!position.GetTurn()), !position.GetTurn())) return false;		

	int staticEval = EvaluatePosition(position);

	if (!(alpha >= staticEval && colour == BLACK) && !(staticEval >= beta && colour == WHITE))
		return false;

	position.ApplyNullMove();
	ABnode* node = CreatePlaceHolderNode(colour, depth);

	if (colour == WHITE)
	{
		Minimize(position, depth - 4, node, beta - 1, beta, false, ALPHABETA);
		position.RevertNullMove();

		if (node->GetScore() >= beta)
		{
			node->SetCutoff(NULL_MOVE_PRUNE);
			parent->SetChild(node);
			return true;
		}
	}

	if (colour == BLACK)
	{
		Maximize(position, depth - 4, node, alpha, alpha + 1, false, ALPHABETA);
		position.RevertNullMove();

		if (node->GetScore() <= alpha)
		{
			node->SetCutoff(NULL_MOVE_PRUNE);
			parent->SetChild(node);
			return true;
		}
	}

	delete node;
	return false;
}

bool FutilityPrune(Position & position, Move & move, int alpha, int beta, int depth, bool colour, int staticEval, bool isfutile)
{
	//return false;

	//if (isfutile)
		//return true;

	//if (depth > 3 || (IsInCheck(position, position.GetKing(WHITE), WHITE)) || (IsInCheck(position, position.GetKing(BLACK), BLACK)))
		//return false;			//Futility pruning only apples to the final two plys, in positions without anyone in check

	if (colour != WHITE)		//we are trying to raise alpha (find a better move than any searched yet)
	{
		if (staticEval + 100 <= alpha)		//if this is true, it would be futile to analyze all the moves in this line as it is bad compared to the best line found so far, so from now on we will just look at all captures promotions and checks
			return true;
	}
	else	
	{
		if (staticEval - 100 >= beta)		
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

/*ABnode * iterativeDeepening(Position & position, int depth, Move & best, int alpha, int beta, int prevScore)
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
}*/

bool InitializeSearchVariables(Position & position, std::vector<Move>& moves, int depth, int & alpha, int & beta, ABnode* parent, bool allowNull, bool colour)
{
	if (CheckForTransposition(position, depth, alpha, beta, parent)) return true;

	moves = GenerateLegalMoves(position);

	if (CheckForCheckmate(position, moves.size(), depth, colour, parent)) return true;
	if (allowNull && NullMovePrune(position, parent, colour, alpha, beta, depth)) return true;

	OrderMoves(moves, position, depth);

	return false;
}

ABnode* SearchToDepth(Position & position, int depth, int alpha, int beta, Move prevBest)
{
	tTable.ResetCount();
	std::vector<Move> moves = GenerateLegalMoves(position);

	//with only one move, we can very quickly just return this move as there is zero benifet to searching the tree below this node
	if (moves.size() == 1)
		return CreateForcedNode(moves[0]);

	OrderMoves(moves, position, depth);
	ABnode* best = CreatePlaceHolderNode(position.GetTurn(), depth);

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
			Minimize(position, depth - 1, node, alpha, beta, true, ALPHABETA);				
		if (position.GetTurn() == WHITE)
			Maximize(position, depth - 1, node, alpha, beta, true, ALPHABETA);
		position.RevertMove(moves[i]);

		//note the move reverting has changed the current turn from before ^
		SetBest(best, node, position.GetTurn());
	}

	return best;
}

void Maximize(Position & position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search)
{
	std::vector<Move> moves;
	if (InitializeSearchVariables(position, moves, depth, alpha, beta, parent, allowNull, WHITE)) return;

	ABnode* best = CreatePlaceHolderNode(WHITE, depth);
	int staticEval = EvaluatePosition(position);

	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* node = CreateBranchNode(moves[i], depth);
		SearchLevels Newsearch = CalculateSearchType(position, moves[i], depth, alpha, beta, WHITE, search);

		position.ApplyMove(moves[i]);
		if (!CheckForDraw(position, node, moves[i], depth))
		{
			if (Newsearch != TERMINATE)
				Minimize(position, depth - 1, node, alpha, beta, true, Newsearch);
			else
			{
				delete node;
				node = new ABnode(moves[i], depth, EXACT, EvaluatePosition(position));		//note this calls evaluate for a second time. possible speedup here
			}
		}
		position.RevertMove(moves[i]);

		SetBest(best, node, WHITE);
		if (CheckForCutoff(alpha, beta, best, BETA_CUTOFF)) break;
	}

	parent->SetChild(best);
	tTable.AddEntry(best->GetMove(), GenerateZobristKey(position), best->GetScore(), depth, best->GetCutoff());
}

void Minimize(Position & position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search)
{
	std::vector<Move> moves;
	if (InitializeSearchVariables(position, moves, depth, alpha, beta, parent, allowNull, BLACK)) return;

	ABnode* best = CreatePlaceHolderNode(BLACK, depth);
	int staticEval = EvaluatePosition(position);

	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* node = CreateBranchNode(moves[i], depth);
		SearchLevels Newsearch = CalculateSearchType(position, moves[i], depth, alpha, beta, BLACK, search);

		position.ApplyMove(moves[i]);
		if (!CheckForDraw(position, node, moves[i], depth))
		{
			if (Newsearch != TERMINATE)
				Maximize(position, depth - 1, node, alpha, beta, true, Newsearch);
			else
			{
				delete node;
				node = new ABnode(moves[i], depth, EXACT, EvaluatePosition(position));		//note this calls evaluate for a second time. possible speedup here
			}
		}
		position.RevertMove(moves[i]);

		SetBest(best, node, BLACK);
		if (CheckForCutoff(alpha, beta, best, ALPHA_CUTOFF)) break;
	}

	parent->SetChild(best);
	tTable.AddEntry(best->GetMove(), GenerateZobristKey(position), best->GetScore(), depth, best->GetCutoff());
}
