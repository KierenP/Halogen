#include "Search.h"

enum SearchLevels
{
	MINMAX,
	ALPHABETA,
	LATE_MOVE_REDUCTION,
	TERMINATE,
	QUIETESSENCE,
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
Move CheckForBestMoveCache(Position& position, int depth);
bool CheckForCheckmate(Position & position, unsigned int size, int depth, bool colour, ABnode* parent);
bool CheckForDraw(Position & position, ABnode*& node, Move& move, int depth);
void SetBest(ABnode*& best, ABnode*& node, bool colour);
bool InitializeSearchVariables(Position& position, std::vector<Move>& moves, int depth, int& alpha, int& beta, ABnode* parent, SearchLevels level, bool InCheck);
void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth);
void MinMax(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search);
void CheckTime();
bool LateMoveReduction(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search);
bool NullMovePrune(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search);
bool IsQuiet(Position& position);
void Quietessence(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search);

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

	std::sort(moves.begin() + swapIndex, moves.end(), [&position, PieceValues](const Move& lhs, const Move& rhs)	//stack exchange answer. Uses 'lambda' expressions. I don't really understand how this workd
		{
			return PieceValues[position.GetSquare(lhs.GetTo())] - PieceValues[position.GetSquare(lhs.GetFrom())] > PieceValues[position.GetSquare(rhs.GetTo())] - PieceValues[position.GetSquare(rhs.GetFrom())];
		});
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

		if (ROOT->GetCutoff() == CHECK_MATE)	//no need to search deeper if this is the case.
			endSearch = true;

		PrintSearchInfo(position, *ROOT, depth, elapsed_ms, endSearch);
		//std::cout << " " << ROOT->GetCutoff();
		std::cout << std::endl;
	
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

		PrintSearchInfo(position, *RankedMoves[0], depth, elapsed_ms, endSearch);
		std::cout << std::endl;

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
	if (depth <= -1)
		return TERMINATE;

	if (index >= 4 && depth >= 4 && !move.IsCapture() && !move.IsPromotion() && !check)
		return LATE_MOVE_REDUCTION;

	if (depth > 1)
		return ALPHABETA;

	return QUIETESSENCE; //at depth of 1, 0
}

bool CheckForTransposition(Position & position, int depth, int & alpha, int & beta, ABnode * parent)
{
	if (tTable.CheckEntry(GenerateZobristKey(position), depth))
	{
		tTable.AddHit();
		TTEntry ttEntry = tTable.GetEntry(GenerateZobristKey(position));
		if (ttEntry.GetCutoff() == EXACT || ttEntry.GetCutoff() == CHECK_MATE)
		{
			parent->SetChild(new ABnode(ttEntry.GetMove(), depth, ttEntry.GetCutoff(), ttEntry.GetScore()));
			return true;
		}
		if (ttEntry.GetCutoff() == ALPHA_CUTOFF)
			alpha = max(alpha, ttEntry.GetScore());
		if (ttEntry.GetCutoff() == BETA_CUTOFF)
			beta = min(beta, ttEntry.GetScore());
	}

	return false;
}

Move CheckForBestMoveCache(Position& position, int depth)
{
	if (tTable.CheckEntry(GenerateZobristKey(position), depth - 1))
		return tTable.GetEntry(GenerateZobristKey(position)).GetMove();
	return Move();
}

bool CheckForCheckmate(Position & position, unsigned int size, int depth, bool colour, ABnode * parent)
{
	if (size != 0)
		return false;

	if (!IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()))
	{
		parent->SetCutoff(THREE_FOLD_REP);
		parent->SetScore(0);
	}
	else
	{
		if (colour == WHITE)
			parent->SetScore(BlackWin - depth);
		if (colour == BLACK)
			parent->SetScore(WhiteWin + depth);

		parent->SetCutoff(CHECK_MATE);
	}

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

bool InitializeSearchVariables(Position & position, std::vector<Move>& moves, int depth, int & alpha, int & beta, ABnode* parent, SearchLevels level, bool InCheck)
{
	if (CheckForTransposition(position, depth, alpha, beta, parent)) return true;

	bool StopSearch = false;

	if (level != QUIETESSENCE || InCheck)
		moves = GenerateLegalMoves(position);
	else
	{
		moves = GenerateLegalHangedCaptures(position);

		if (moves.size() == 0)
		{
			//not in check and no hanging pieces
			StopSearch = true;
			moves = GenerateLegalMoves(position);	//with zero captures we need to make sure its not actually stalemate. This SHOULD be rare and quick to check for anyways I think
		}
	}

	//check for possible check mate / stalemate in this position
	if (moves.size() == 0)
	{
		if (!InCheck)
		{
			parent->SetCutoff(THREE_FOLD_REP);
			parent->SetScore(0);
		}
		else
		{
			if (position.GetTurn() == WHITE)
				parent->SetScore(BlackWin - depth);	//sooner checkmates are better
			if (position.GetTurn() == BLACK)
				parent->SetScore(WhiteWin + depth);

			parent->SetCutoff(CHECK_MATE);
		}

		return true;
	}

	if (StopSearch)
	{//not in check and no hanging pieces, also not in checkmate or stalemate so do static evaluation
		parent->SetCutoff(EXACT);
		parent->SetScore(EvaluatePosition(position));
		return true;
	}

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

bool IsQuiet(Position& position)
{
	if (position.GetTurn() == BLACK)
	{
		uint64_t KnightThreats = position.GetAttackTable(BLACK_KNIGHT) & (position.GetPieceBB(WHITE_ROOK) | position.GetPieceBB(WHITE_QUEEN));
		if (KnightThreats != EMPTY) return false;

		uint64_t BishopThreats = position.GetAttackTable(BLACK_BISHOP) & (position.GetPieceBB(WHITE_ROOK) | position.GetPieceBB(WHITE_QUEEN));
		if (BishopThreats != EMPTY) return false;

		uint64_t RookThreats = position.GetAttackTable(BLACK_ROOK) & (position.GetPieceBB(WHITE_QUEEN));
		if (RookThreats != EMPTY) return false;
	}

	if (position.GetTurn() == WHITE)
	{
		uint64_t KnightThreats = position.GetAttackTable(WHITE_KNIGHT) & (position.GetPieceBB(BLACK_ROOK) | position.GetPieceBB(BLACK_QUEEN));
		if (KnightThreats != EMPTY) return false;

		uint64_t BishopThreats = position.GetAttackTable(WHITE_BISHOP) & (position.GetPieceBB(BLACK_ROOK) | position.GetPieceBB(BLACK_QUEEN));
		if (BishopThreats != EMPTY) return false;

		uint64_t RookThreats = position.GetAttackTable(WHITE_ROOK) & (position.GetPieceBB(BLACK_QUEEN));
		if (RookThreats != EMPTY) return false;
	}

	return true;
}

void Quietessence(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search)
{
	CurrentTime = clock();
	CheckTime();

	std::vector<Move> moves;
	bool InCheck = IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn());
	if (InitializeSearchVariables(position, moves, depth, alpha, beta, parent, search, InCheck)) return;

	//if no captures turn out to be any good, then assume there is a quiet move alternative and we cut the branch at the parent
	//note we already checked for checkmate, so its a safe assumption that if not in check there is another possible move 
	ABnode* best;

	if (!IsInCheck)
		best = CreateLeafNode(position, depth);
	else
		best = CreatePlaceHolderNode(position.GetTurn(), depth);

	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* node = CreateBranchNode(moves[i], depth);
		SearchLevels Newsearch = CalculateSearchType(position, moves[i], depth, InCheck, i);

		position.ApplyMove(moves[i]);
		if (!CheckForDraw(position, node, moves[i], depth))
		{
			if ((Newsearch == QUIETESSENCE || IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn())) && !AbortSearch)
				Quietessence(position, depth - 1, node, alpha, beta, true, Newsearch);
			else
			{
				node->SetCutoff(EXACT);
				node->SetScore(EvaluatePosition(position));
			}
		}
		position.RevertMove(moves[i]);

		SetBest(best, node, position.GetTurn());
		if (CheckForCutoff(alpha, beta, best, position.GetTurn() ? BETA_CUTOFF : ALPHA_CUTOFF)) break;
	}

	if (best->GetMove() == Move())	//if none of the captures were any good...
	{
		parent = CreateLeafNode(position, depth);
	}
	else
	{
		parent->SetChild(best);
		tTable.AddEntry(TTEntry(best->GetMove(), GenerateZobristKey(position), best->GetScore(), depth, best->GetCutoff()));
	}
}

void MinMax(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search)
{
	CurrentTime = clock();
	CheckTime();

	std::vector<Move> moves;
	bool InCheck = IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn());

	if (InitializeSearchVariables(position, moves, depth, alpha, beta, parent, search, InCheck)) return;

	ABnode* best = CreatePlaceHolderNode(position.GetTurn(), depth);

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

			if (!InCheck && allowNull)
				if (NullMovePrune(position, depth, node, alpha, beta, false, Newsearch))
					Cut = true;

			if (!Cut)
				if (Newsearch == QUIETESSENCE || AbortSearch || Newsearch == TERMINATE)
					Quietessence(position, depth - 1, node, alpha, beta, true, Newsearch);
				else
					MinMax(position, depth - 1, node, alpha, beta, true, Newsearch);
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
		if (EvaluatePosition(position) > alpha - 350)
			return false;

		MinMax(position, depth - 2, parent, alpha, alpha + 1, true, search);	

		if (parent->GetCutoff() == ALPHA_CUTOFF)
			return true;
		return false;
	}

	if (position.GetTurn() == WHITE)
	{ //we just had a white move played and it is now blacks turn to move
		if (EvaluatePosition(position) < beta + 350)
			return false;

		MinMax(position, depth - 2, parent, beta - 1, beta, true, search);		

		if (parent->GetCutoff() == BETA_CUTOFF)
			return true;
		return false;
	}
}

bool NullMovePrune(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search)
{
	if (position.GetTurn() == BLACK)
	{ //we just had a white move played and it is now blacks turn to move
		if (EvaluatePosition(position) < beta + 350)
			return false;

		position.ApplyNullMove();
		MinMax(position, depth - 3, parent, beta - 1, beta, false, search);
		position.RevertNullMove();
		return (parent->GetCutoff() == BETA_CUTOFF);
	}

	if (position.GetTurn() == WHITE)
	{ //we just had a white move played and it is now blacks turn to move
		if (EvaluatePosition(position) > alpha - 350)
			return false;

		position.ApplyNullMove();
		MinMax(position, depth - 3, parent, alpha, alpha + 1, false, search);
		position.RevertNullMove();
		return (parent->GetCutoff() == ALPHA_CUTOFF);
	}

	return false;
}
