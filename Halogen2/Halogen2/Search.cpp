#include "Search.h"

enum SearchLevels
{
	MINMAX,
	ALPHABETA,
	TERMINATE,
	QUIETESSENCE,			//captures and all moves if in check
	LEAF_SEARCH,				//only look at captures of hanged pieces
	CHECK_EXTENSION
};

TranspositionTable tTable;
clock_t SearchBegin;
clock_t CurrentTime;

int AllowedSearchTime = 0;	//in ms. 
bool AbortSearch = false;	//Periodically check CurrentTime - SearchBegin and if greater than some value then set this to true.

ABnode* SearchToDepth(Position& position, int depth, int alpha, int beta);
std::vector<ABnode*> SearchDebug(Position& position, int depth, int alpha, int beta);		//Return the best 4 moves in order from this position
void PrintSearchInfo(Position& position, ABnode& node, unsigned int depth, double Time, bool isCheckmate);
void SwapMoves(std::vector<Move>& moves, unsigned int a, unsigned int b);
SearchLevels CalculateSearchType(Position& position, int depth, bool check);
bool CheckForCutoff(int& alpha, int& beta, ABnode* best, unsigned int cutoff);
bool CheckForTransposition(Position& position, int depth, int& alpha, int& beta, ABnode* parent);
bool CheckForDraw(ABnode*& node, Move& move, int depth, Position& position);
void SetBest(ABnode*& best, ABnode*& node, bool colour);
bool InitializeSearchVariables(Position& position, std::vector<Move>& moves, int depth, int& alpha, int& beta, ABnode* parent, SearchLevels level, bool InCheck);
void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth);
void MinMax(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search);
void CheckTime();
bool NullMovePrune(Position& position, int depth, ABnode* parent, int alpha, int beta, SearchLevels search);
bool LateMoveReduction(Position& position, int depth, ABnode* parent, int alpha, int beta, SearchLevels search);
void Quietessence(Position& position, int depth, ABnode* parent, int alpha, int beta, SearchLevels search);
ABnode* SearchToDepthAspiration(Position& position, int depth, int score);

void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth)
{
	/*
	We want to order the moves such that the best moves are more likely to be further towards the front.
	*/
	unsigned int swapIndex = 0;
	std::vector<int> PieceValues = { 1, 3, 3, 5, 9, 100, 1, 3, 3, 5, 9, 100, 1 };	//relative values. Note empty is a pawn. This is explained below

	//move previously cached position to front
	if (tTable.CheckEntry(position.GetZobristKey(), searchDepth - 1))
	{
		Move bestprev = tTable.GetEntry(position.GetZobristKey()).GetMove();
		for (int i = swapIndex; i < moves.size(); i++)
		{
			if (bestprev == moves.at(i))
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
		if (moves.at(i).IsPromotion())
		{
			SwapMoves(moves, i, swapIndex);
			swapIndex++;
		}
	}

	int captures = 0;

	//move all captures behind it
	for (int i = swapIndex; i < moves.size(); i++)
	{
		if (moves.at(i).IsCapture())
		{
			captures++;
			SwapMoves(moves, i, swapIndex);
			swapIndex++;
		}
	}

	//stack exchange answer. Uses 'lambda' expressions. I don't really understand how this works
	//Note if it is an en passant, then there is nothing in the square the piece is moving to and this caused an out of bounds index and garbage memory access.
	//It took me about 5 hours to figure it out. The workaround was adding a 1 to the end of the pieceValues vector.
	/*
	EDIT: about 6 hours more googling and debugging it turns out that std::sort is an unstable sort which was causing non-deterministic behaviour.
	I have replaced this with std::stable_sort which should hopefully fix the problem
	*/


	std::stable_sort(moves.begin() + swapIndex - captures, moves.begin() + swapIndex, [&position, PieceValues](const Move& lhs, const Move& rhs)
		{
			return PieceValues.at(position.GetSquare(lhs.GetTo())) - PieceValues.at(position.GetSquare(lhs.GetFrom())) > PieceValues.at(position.GetSquare(rhs.GetTo())) - PieceValues.at(position.GetSquare(rhs.GetFrom()));
		});
}

Move SearchPosition(Position& position, int allowedTimeMs)
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
		ABnode* ROOT = SearchToDepthAspiration(position, depth, score);
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
		std::cout << " " << ROOT->GetCutoff();
		std::cout << std::endl;

		Best = ROOT->GetMove();
		score = ROOT->GetScore();
		delete ROOT;
	}

	return Best;
}

std::vector<Move> SearchBenchmark(Position& position, int allowedTimeMs)
{
	AbortSearch = false;
	AllowedSearchTime = allowedTimeMs;
	SearchBegin = clock();

	tTable.SetAllAncient();

	std::vector<Move> moves = GenerateLegalMoves(position);
	std::vector<Move> best;

	Move Best[4];

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

		if (!AbortSearch)
		{	//stick with what we previously found to be best if we had to abort.
			best.clear();
			best.push_back(RankedMoves[0]->GetMove());
			best.push_back(RankedMoves[1]->GetMove());
			best.push_back(RankedMoves[2]->GetMove());
			best.push_back(RankedMoves[3]->GetMove());

			PrintSearchInfo(position, *RankedMoves[0], depth, elapsed_ms, endSearch);
			std::cout << std::endl;
		}

		for (int i = 0; i < RankedMoves.size(); i++)
		{
			//PrintSearchInfo(position, *RankedMoves.at(i), depth, elapsed_ms, false);
			//std::cout << " " << RankedMoves.at(i)->GetCutoff();
			//std::cout << std::endl;
			delete RankedMoves.at(i);
		}
	}

	return best;
}

void PrintSearchInfo(Position& position, ABnode& node, unsigned int depth, double Time, bool isCheckmate)
{
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
		<< " nps " << int(NodeCount / (Time) * 1000)
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
	if (a >= moves.size() || b >= moves.size()) throw std::invalid_argument("Bad paramiter to SetSquare()");

	Move temp = moves[b];
	moves[b] = moves[a];
	moves[a] = temp;
}

SearchLevels CalculateSearchType(Position& position, int depth, bool check)
{
	if (depth > 1)				// (inf, 1)
		return ALPHABETA;

	if (depth <= 1 && depth > -1)	// [1, -1)
	{
		if (check)
			return CHECK_EXTENSION;

		if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()))	//if I just got put in check by that move
			return CHECK_EXTENSION;

		return QUIETESSENCE;
	}

	/*if (depth <= -1 && depth > -3)	// [-1, 3)
	{
		if (check)
			return CHECK_EXTENSION;

		if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()))	//if I just got put in check by that move
			return CHECK_EXTENSION;
	}*/

	if (depth == -1)
		return LEAF_SEARCH;

	if (depth <= -2)
		return TERMINATE;

	return TERMINATE;
}

bool CheckForTransposition(Position& position, int depth, int& alpha, int& beta, ABnode* parent)
{
	if (tTable.CheckEntry(position.GetZobristKey(), depth))
	{
		tTable.AddHit();
		TTEntry ttEntry = tTable.GetEntry(position.GetZobristKey());
		if (ttEntry.GetCutoff() == EXACT || ttEntry.GetCutoff() == QUIETESSENCE)
		{
			parent->SetChild(new ABnode(ttEntry.GetMove(), depth, ttEntry.GetCutoff(), ttEntry.GetScore()));
			return true;
		}

		if (ttEntry.GetCutoff() == BETA_CUTOFF)
		{
			//the score is at least the saved score
			if (position.GetTurn() == WHITE)
			{
				if (ttEntry.GetScore() >= beta)
				{
					parent->SetChild(new ABnode(ttEntry.GetMove(), depth, ttEntry.GetCutoff(), ttEntry.GetScore()));
					return true;
				}
			}
		}

		if (ttEntry.GetCutoff() == ALPHA_CUTOFF)
		{
			//the score is at most the saved score
			if (position.GetTurn() == BLACK)
			{
				if (ttEntry.GetScore() <= alpha)
				{
					parent->SetChild(new ABnode(ttEntry.GetMove(), depth, ttEntry.GetCutoff(), ttEntry.GetScore()));
					return true;
				}
			}
		}
	}

	return false;
}

bool CheckForDraw(ABnode*& node, Move& move, int depth, Position& position)
{
	int rep = 0;
	uint64_t current = position.GetZobristKey();

	for (int i = 0; i < PreviousKeys.size(); i++)
	{
		if (PreviousKeys.at(i) == current)
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

bool CheckForCutoff(int& alpha, int& beta, ABnode* best, unsigned int cutoff)
{
	if (best->GetMove() == Move())
		return false;

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

bool InitializeSearchVariables(Position& position, std::vector<Move>& moves, int depth, int& alpha, int& beta, ABnode* parent, SearchLevels level, bool InCheck)
{
	if (CheckForTransposition(position, depth, alpha, beta, parent)) return true;

	if (level == QUIETESSENCE)
		moves = GenerateLegalCaptures(position);
	else if (level == LEAF_SEARCH)
		moves = GenerateLegalHangedCaptures(position);
	else
		moves = GenerateLegalMoves(position);

	if (moves.size() == 0)
	{
		if (level == QUIETESSENCE || level == LEAF_SEARCH)
		{
			parent->SetCutoff(EXACT);
			parent->SetScore(EvaluatePosition(position));
			return true;
		}

		if (InCheck)
		{
			if (position.GetTurn() == WHITE)
				parent->SetScore(BlackWin - depth);	//sooner checkmates are better
			if (position.GetTurn() == BLACK)
				parent->SetScore(WhiteWin + depth);

			parent->SetCutoff(CHECK_MATE);
		}
		else
		{
			parent->SetCutoff(THREE_FOLD_REP);
			parent->SetScore(0);
		}

		return true;
	}

	OrderMoves(moves, position, depth);
	return false;
}

ABnode* SearchToDepth(Position& position, int depth, int alpha, int beta)
{
	tTable.ResetHitCount();

	std::vector<Move> moves = GenerateLegalMoves(position);
	ABnode* best = CreatePlaceHolderNode(position.GetTurn(), depth);

	OrderMoves(moves, position, depth);

	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* node = CreateBranchNode(moves.at(i), depth);

		position.ApplyMove(moves.at(i));
		MinMax(position, depth - 1, node, alpha, beta, true, ALPHABETA);
		position.RevertMove(moves.at(i));

		if (AbortSearch)	//go with what was previously found to be the best move so far, as he had to stop the search early.
		{
			delete node;
			break;
		}

		SetBest(best, node, position.GetTurn());
	}

	tTable.AddEntry(TTEntry(best->GetMove(), position.GetZobristKey(), best->GetScore(), depth, best->GetCutoff()));
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
		ABnode* node = CreateBranchNode(moves.at(i), depth);
		position.ApplyMove(moves.at(i));
		MinMax(position, depth - 1, node, alpha, beta, true, ALPHABETA);
		position.RevertMove(moves.at(i));
		MoveNodes.push_back(node);
	}

	//Quick and dirty swap sort
	bool swapped = false;
	do
	{
		swapped = false;
		for (int i = 0; i < MoveNodes.size() - 1; i++)
		{
			if ((position.GetTurn() == WHITE && MoveNodes.at(i)->GetScore() < MoveNodes[i + 1]->GetScore()) || (position.GetTurn() == BLACK && MoveNodes.at(i)->GetScore() > MoveNodes[i + 1]->GetScore()))
			{
				std::iter_swap(MoveNodes.begin() + i, MoveNodes.begin() + i + 1);
				swapped = true;
			}
		}
	} while (swapped);

	tTable.AddEntry(TTEntry(MoveNodes[0]->GetMove(), position.GetZobristKey(), MoveNodes[0]->GetScore(), depth, MoveNodes[0]->GetCutoff()));
	return MoveNodes;
}

void Quietessence(Position& position, int depth, ABnode* parent, int alpha, int beta, SearchLevels search)
{
	CurrentTime = clock();
	CheckTime();

	std::vector<Move> moves;
	bool InCheck = IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn());
	if (InitializeSearchVariables(position, moves, depth, alpha, beta, parent, search, InCheck)) return;

	//if no captures turn out to be any good, then assume there is a quiet move alternative and we cut the branch at the parent
	//note we already checked for checkmate, so its a safe assumption that if not in check there is another possible move 
	ABnode* best;

	if (!InCheck)
		best = CreateLeafNode(position, depth);
	else
		best = CreatePlaceHolderNode(position.GetTurn(), depth);

	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* node = CreateBranchNode(moves.at(i), depth);

		position.ApplyMove(moves.at(i));
		SearchLevels Newsearch = CalculateSearchType(position, depth, InCheck);

		if (!CheckForDraw(node, moves.at(i), depth, position))
		{
			if (Newsearch == TERMINATE || AbortSearch)
			{
				node->SetCutoff(QUIESSENCE_NODE);
				node->SetScore(EvaluatePosition(position));
			}
			else
				Quietessence(position, depth - 1, node, alpha, beta, Newsearch);
		}
		position.RevertMove(moves.at(i));

		SetBest(best, node, position.GetTurn());
		if (CheckForCutoff(alpha, beta, best, position.GetTurn() ? BETA_CUTOFF : ALPHA_CUTOFF)) break;
	}

	if (best->GetMove() == Move())	//if none of the captures were any good...
	{	//we assume there exists another quiet move we could have done instead
		parent->SetCutoff(EXACT);
		parent->SetScore(EvaluatePosition(position));
		delete best;
	}
	else
	{
		parent->SetChild(best);
		tTable.AddEntry(TTEntry(best->GetMove(), position.GetZobristKey(), best->GetScore(), depth, best->GetCutoff()));
	}
}

ABnode* SearchToDepthAspiration(Position& position, int depth, int score)
{
	int windowWidth = 25;

	int alpha = score - windowWidth;
	int beta = score + windowWidth;
	int betaIncrements = 0;
	int AlphaIncrements = 0;

	tTable.ResetHitCount();

	std::vector<Move> moves = GenerateLegalMoves(position);
	ABnode* best = CreatePlaceHolderNode(position.GetTurn(), depth);

	OrderMoves(moves, position, depth);
	bool Redo = true;

	while (Redo && !AbortSearch)
	{
		Redo = false;

		for (int i = 0; i < moves.size(); i++)
		{
			ABnode* node = CreateBranchNode(moves.at(i), depth);

			position.ApplyMove(moves.at(i));
			MinMax(position, depth - 1, node, alpha, beta, true, ALPHABETA);
			position.RevertMove(moves.at(i));

			if (AbortSearch)	//go with what was previously found to be the best move so far, as he had to stop the search early.	
			{
				delete node;
				break;
			}

			SetBest(best, node, position.GetTurn());
		}

		if (best->GetCutoff() == BETA_CUTOFF)
		{
			std::cout << "info Beta-Cutoff" << best->GetScore() << std::endl;
			betaIncrements++;
			beta = HighINF;
			delete best;
			best = CreatePlaceHolderNode(position.GetTurn(), depth);
			Redo = true;
		}
		if (best->GetCutoff() == ALPHA_CUTOFF)
		{
			std::cout << "info Alpha-Cutoff" << best->GetScore() << std::endl;
			AlphaIncrements++;
			alpha = LowINF;
			delete best;
			best = CreatePlaceHolderNode(position.GetTurn(), depth);
			Redo = true;
		}
	}

	tTable.AddEntry(TTEntry(best->GetMove(), position.GetZobristKey(), best->GetScore(), depth, best->GetCutoff()));
	return best;
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
		ABnode* node = CreateBranchNode(moves.at(i), depth);
		position.ApplyMove(moves.at(i));
		SearchLevels Newsearch = CalculateSearchType(position, depth, InCheck);

		if (!CheckForDraw(node, moves.at(i), depth, position))
		{
			bool Cut = false;

			//Late move reductions
			if (i >= 4 && depth >= 4 && !moves.at(i).IsCapture() && !moves.at(i).IsPromotion() && !InCheck)
				if (LateMoveReduction(position, depth, node, alpha, beta, Newsearch))
					Cut = true;

			//null move prune
			if (!IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()) && allowNull && depth >= 4)
				if (NullMovePrune(position, depth, node, alpha, beta, Newsearch))
					Cut = true;

			if (!Cut)
				if (AbortSearch || Newsearch != ALPHABETA)
					Quietessence(position, depth - 1, node, alpha, beta, Newsearch);
				else
					MinMax(position, depth - 1, node, alpha, beta, true, Newsearch);
		}
		position.RevertMove(moves.at(i));

		SetBest(best, node, position.GetTurn());
		if (CheckForCutoff(alpha, beta, best, position.GetTurn() ? BETA_CUTOFF : ALPHA_CUTOFF)) break;
	}

	parent->SetChild(best);
	tTable.AddEntry(TTEntry(best->GetMove(), position.GetZobristKey(), best->GetScore(), depth, best->GetCutoff()));
}

void CheckTime()
{
	if ((double(CurrentTime) - double(SearchBegin)) / CLOCKS_PER_SEC * 1000 > AllowedSearchTime)
		AbortSearch = true;
}

bool NullMovePrune(Position& position, int depth, ABnode* parent, int alpha, int beta, SearchLevels search)
{
	if (position.GetTurn() == WHITE)
	{ //we just had a black move played and it is now whites turn to move, which we are skipping to see if its still good for white 'beta cutoff'
		position.ApplyNullMove();
		MinMax(position, depth - 3, parent, beta - 1, beta, false, search);
		position.RevertNullMove();
		return (parent->GetCutoff() == BETA_CUTOFF);
	}

	if (position.GetTurn() == BLACK)
	{
		position.ApplyNullMove();
		MinMax(position, depth - 3, parent, alpha, alpha + 1, false, search);
		position.RevertNullMove();
		return (parent->GetCutoff() == ALPHA_CUTOFF);
	}

	return false;
}

bool LateMoveReduction(Position& position, int depth, ABnode* parent, int alpha, int beta, SearchLevels search)
{
	if (position.GetTurn() == BLACK)
	{ 
		MinMax(position, depth - 2, parent, beta - 1, beta, true, search);
		return (parent->GetCutoff() == BETA_CUTOFF);
	}

	if (position.GetTurn() == WHITE)
	{
		MinMax(position, depth - 2, parent, alpha, alpha + 1, true, search);
		return (parent->GetCutoff() == ALPHA_CUTOFF);
	}

	return false;
}
