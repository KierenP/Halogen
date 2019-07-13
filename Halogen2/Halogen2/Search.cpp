#include "Search.h"

enum SearchLevels
{
	ALPHABETA,
	TERMINATE,
	QUIETESSENCE,			//captures and all moves if in check
	LEAF_SEARCH,				//only look at captures of hanged pieces
	CHECK_EXTENSION
};

TranspositionTable tTable;
SearchTimeManage timeManage;

//Search functions
ABnode* SearchToDepthAspiration(Position& position, int depth, int score);
void AspirationWindowAdjust(ABnode*& best, int& betaInc, int& beta, int score, int windowWidth, Position& position, int depth, bool& Redo, int& alphaInc, int& alpha);
std::vector<ABnode*> SearchDebug(Position& position, int depth, int alpha, int beta);		//Return the best 4 moves in order from this position
void AlphaBeta(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search);
void Quietessence(Position& position, int depth, ABnode* parent, int alpha, int beta, SearchLevels search);

//Search aid functions
SearchLevels CalculateSearchType(Position& position, int depth, bool check);
void GetSearchMoves(SearchLevels level, std::vector<Move>& moves, Position& position);
bool InitializeSearchVariables(Position& position, std::vector<Move>& moves, int depth, int& alpha, int& beta, ABnode* parent, SearchLevels level, bool InCheck);
void SetBest(ABnode*& best, ABnode*& node, bool colour);
bool CheckForCutoff(int& alpha, int& beta, ABnode* best, unsigned int cutoff);
bool CheckForTransposition(Position& position, int depth, int& alpha, int& beta, ABnode* parent);
bool CheckForDraw(ABnode*& node, int depth, Position& position);
void EndSearch(SearchLevels level, ABnode* parent, Position& position, bool InCheck, int depth);
void AspirationWindowAdjust(ABnode*& best, int& betaInc, int& beta, int score, int windowWidth, Position& position, int depth, bool& Redo, int& alphaInc, int& alpha);

//Branch pruning techniques
bool IsFutile(Position& position, Move& move, int alpha, int beta);
bool LateMoveReduction(Position& position, int depth, ABnode* parent, int alpha, int beta, SearchLevels search);
bool NullMovePrune(Position& position, int depth, ABnode* parent, int alpha, int beta, SearchLevels search);

//Utility functions
void SwapMoves(std::vector<Move>& moves, unsigned int a, unsigned int b);
void PrintSearchInfo(Position& position, ABnode& node, unsigned int depth, double Time, bool isCheckmate);
int int_pow(int base, int exp); //Node pow() as part of the std uses the double data type and hence is not suitable for integer powers due to the possibility of 2 ^ 5 = 31.9999999999997 = 31 for example
void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth);
void PrintBestMove(Move& Best);


//stolen from stack-overflow. Uses the 'exponentiation by squaring' technique which is faster
int int_pow(int base, int exp)
{
	int result = 1;
	while (exp)
	{
		if (exp & 1)
			result *= base;
		exp /= 2;
		base *= base;
	}
	return result;
}

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

	honestly if you add up all the time saved using this over a custom sort written by me, 
	it would have been quicker if I never tried to be clever in the first place
	*/

	//Sort the captures
	std::stable_sort(moves.begin() + swapIndex - captures, moves.begin() + swapIndex, [position, PieceValues](const Move& lhs, const Move& rhs)
		{
			return PieceValues.at(position.GetSquare(lhs.GetTo())) - PieceValues.at(position.GetSquare(lhs.GetFrom())) > PieceValues.at(position.GetSquare(rhs.GetTo())) - PieceValues.at(position.GetSquare(rhs.GetFrom()));
		});
}

Move SearchPosition(Position& position, int allowedTimeMs)
{
	timeManage.StartSearch(allowedTimeMs);

	tTable.SetAllAncient();
	std::vector<Move> moves = GenerateLegalMoves(position);

	if (moves.size() == 1)	//If there is only one legal move, then we just play that move immediantly
	{
		PrintBestMove(moves[0]);
		return moves[0];
	}

	Move Best;
	int score = EvaluatePosition(position);
	bool endSearch = false;

	//abort if I have used up more than half the time, if we have used up less than half it will try and search another ply.
	for (int depth = 1; !timeManage.AbortSearch() && !endSearch && timeManage.ContinueSearch(); depth++)
	{
		NodeCount = 0;
		Timer searchTime;

		searchTime.Start();
		ABnode* ROOT = SearchToDepthAspiration(position, depth, score);

		if (timeManage.AbortSearch()) //stick with what we previously found to be best if we had to abort.
		{	
			delete ROOT;
			break;
		}

		if (ROOT->GetCutoff() == CHECK_MATE_CUTOFF)	//no need to search deeper if this is the case.
			endSearch = true;

		PrintSearchInfo(position, *ROOT, depth, searchTime.ElapsedMs(), endSearch);

		Best = ROOT->GetMove();
		score = ROOT->GetScore();
		delete ROOT;
	}

	PrintBestMove(Best);

	return Best;
}

void PrintBestMove(Move& Best)
{
	std::cout << "bestmove ";
	Best.Print();
	std::cout << std::endl;
}

void PrintSearchInfo(Position& position, ABnode& node, unsigned int depth, double Time, bool isCheckmate)
{
	std::cout
		<< "info depth " << depth																				//the depth of search
		<< " seldepth " << node.CountNodeChildren();															//the selective depth (for example searching further for checks and captures)

	if (isCheckmate)
	{
		if ((node.GetScore() > 0 && position.GetTurn() == WHITE) || (node.GetScore() < 0 && position.GetTurn() == BLACK))
			std::cout << " score mate " << (max(depth, node.CountNodeChildren()) + 1) / 2;	//mating opponent	
		else
			std::cout << " score mate " << -int(max(depth, node.CountNodeChildren()) + 1) / 2;	//getting mated
	}
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
	std::cout << std::endl;
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
	if (depth > 1)				
		return ALPHABETA;

	if (depth <= 1 && depth > -3)	
	{
		if (check)
			return CHECK_EXTENSION;

		if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()))	//if I just got put in check by that move
			return CHECK_EXTENSION;
	}

	if (depth <= 1 && depth > -1) return QUIETESSENCE;

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

		if (ttEntry.GetCutoff() == EXACT_CUTOFF || ttEntry.GetCutoff() == QUIESSENCE_NODE_CUTOFF)
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

bool CheckForDraw(ABnode*& node, int depth, Position& position)
{
	int rep = 1;
	uint64_t current = position.GetZobristKey();

	for (int i = 0; i < PreviousKeys.size(); i++)
	{
		if (PreviousKeys.at(i) == current)
			rep++;
	}

	if (rep >= 3)
	{
		node->SetScore(0);
		node->SetCutoff(THREE_FOLD_REP_CUTOFF);
		return true;
	}
	return false;
}

bool CheckForCutoff(int& alpha, int& beta, ABnode* best, unsigned int cutoff)
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


bool InitializeSearchVariables(Position& position, std::vector<Move>& moves, int depth, int& alpha, int& beta, ABnode* parent, SearchLevels level, bool InCheck)
{
	if (CheckForDraw(parent, depth, position)) return true;
	if (CheckForTransposition(position, depth, alpha, beta, parent)) return true;

	GetSearchMoves(level, moves, position);

	if (moves.size() == 0)
	{
		EndSearch(level, parent, position, InCheck, depth);
		return true;
	}

	OrderMoves(moves, position, depth);
	return false;
}

void EndSearch(SearchLevels level, ABnode* parent, Position& position, bool InCheck, int depth)
{
	if (level == QUIETESSENCE || level == LEAF_SEARCH)
	{
		parent->SetCutoff(EXACT_CUTOFF);
		parent->SetScore(EvaluatePosition(position));
	}

	else if (InCheck)
	{
		if (position.GetTurn() == WHITE)
			parent->SetScore(BlackWin - depth);	//sooner checkmates are better
		if (position.GetTurn() == BLACK)
			parent->SetScore(WhiteWin + depth);

		parent->SetCutoff(CHECK_MATE_CUTOFF);
	}
	else
	{
		parent->SetCutoff(THREE_FOLD_REP_CUTOFF);
		parent->SetScore(0);
	}
}

void GetSearchMoves(SearchLevels level, std::vector<Move>& moves, Position& position)
{
	switch (level)
	{
	case ALPHABETA:
		moves = GenerateLegalMoves(position);
		break;
	case TERMINATE:
		throw std::invalid_argument("YEET");
		break;
	case QUIETESSENCE:
		moves = GenerateLegalCaptures(position);
		break;
	case LEAF_SEARCH:
		moves = GenerateLegalHangedCaptures(position);
		break;
	case CHECK_EXTENSION:
		moves = GenerateLegalMoves(position);
		break;
	default:
		break;
	}
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
		AlphaBeta(position, depth - 1, node, alpha, beta, true, ALPHABETA);
		position.RevertMove(moves.at(i));
		MoveNodes.push_back(node);
	}

	std::stable_sort(MoveNodes.begin(), MoveNodes.end(), [position](const ABnode* lhs, const ABnode* rhs)
		{
			if ((position.GetTurn() == WHITE && lhs->GetScore() > rhs->GetScore())) return true;
			if ((position.GetTurn() == BLACK && lhs->GetScore() < rhs->GetScore())) return true;
			return false;
		});

	tTable.AddEntry(MoveNodes[0]->GetMove(), position.GetZobristKey(), MoveNodes[0]->GetScore(), depth, MoveNodes[0]->GetCutoff());
	return MoveNodes;
}

void Quietessence(Position& position, int depth, ABnode* parent, int alpha, int beta, SearchLevels search)
{
	if (timeManage.AbortSearch()) return;

	std::vector<Move> moves;
	bool InCheck = IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn());
	if (InitializeSearchVariables(position, moves, depth, alpha, beta, parent, search, InCheck)) return; //we hit a transposition/checkmate/draw or other ternimal node

	//if no captures turn out to be any good, then assume there is a quiet move alternative and we cut the branch at the parent
	//note we already checked for checkmate, so its a safe assumption that if not in check there is another possible move 
	ABnode* best;

	if (!InCheck)
		best = CreateLeafNode(position, depth);														//possible zanzuag issue, see below
	else
		best = CreatePlaceHolderNode(position.GetTurn(), depth);

	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* node = CreateBranchNode(moves.at(i), depth);

		position.ApplyMove(moves.at(i));
		SearchLevels Newsearch = CalculateSearchType(position, depth, InCheck);

		if (Newsearch == TERMINATE)
		{
			node->SetCutoff(QUIESSENCE_NODE_CUTOFF);
			node->SetScore(EvaluatePosition(position));
		}
		else
			Quietessence(position, depth - 1, node, alpha, beta, Newsearch);
	
		position.RevertMove(moves.at(i));

		SetBest(best, node, position.GetTurn());
		if (CheckForCutoff(alpha, beta, best, position.GetTurn() ? BETA_CUTOFF : ALPHA_CUTOFF)) break; //if in zanzuag, this may cause a cutoff if best was a leaf node
	}

	if (best->GetMove() == Move())	//if none of the captures were any good we assume there exists another quiet move we could have done instead
	{	
		parent->SetCutoff(EXACT_CUTOFF);
		parent->SetScore(EvaluatePosition(position));
		delete best;
	}
	else
	{
		parent->SetChild(best);
		tTable.AddEntry(best->GetMove(), position.GetZobristKey(), best->GetScore(), depth, best->GetCutoff());
	}
}

ABnode* SearchToDepthAspiration(Position& position, int depth, int score)
{
	int windowWidth = 25;

	int alphaInc = 0;
	int betaInc = 0;
	int alpha = score - windowWidth * int_pow(2, alphaInc);
	int beta = score + windowWidth * int_pow(2, betaInc);

	tTable.ResetHitCount();

	std::vector<Move> moves = GenerateLegalMoves(position);
	ABnode* best = CreatePlaceHolderNode(position.GetTurn(), depth);

	OrderMoves(moves, position, depth);
	bool Redo = true;

	while (Redo && !timeManage.AbortSearch())
	{
		Redo = false;

		for (int i = 0; i < moves.size(); i++)
		{
			ABnode* node = CreateBranchNode(moves.at(i), depth);

			position.ApplyMove(moves.at(i));
			AlphaBeta(position, depth - 1, node, alpha, beta, true, ALPHABETA);
			position.RevertMove(moves.at(i));

			if (timeManage.AbortSearch())	//go with what was previously found to be the best move so far, as he had to stop the search early.	
			{
				delete node;
				break;
			}

			SetBest(best, node, position.GetTurn());
		}

		AspirationWindowAdjust(best, betaInc, beta, score, windowWidth, position, depth, Redo, alphaInc, alpha);
	}

	tTable.AddEntry(best->GetMove(), position.GetZobristKey(), best->GetScore(), depth, best->GetCutoff());
	return best;
}

void AspirationWindowAdjust(ABnode*& best, int& betaInc, int& beta, int score, int windowWidth, Position& position, int depth, bool& Redo, int& alphaInc, int& alpha)
{
	if (best->GetCutoff() == BETA_CUTOFF)
	{
		std::cout << "info Beta-Cutoff" << best->GetScore() << std::endl;
		betaInc++;
		beta = score + windowWidth * int_pow(2, betaInc);	//double the distance
		delete best;
		best = CreatePlaceHolderNode(position.GetTurn(), depth);
		Redo = true;
	}

	if (best->GetCutoff() == ALPHA_CUTOFF)
	{
		std::cout << "info Alpha-Cutoff" << best->GetScore() << std::endl;
		alphaInc++;
		alpha = score - windowWidth * int_pow(2, alphaInc);	//double the distance
		delete best;
		best = CreatePlaceHolderNode(position.GetTurn(), depth);
		Redo = true;
	}
}

bool IsFutile(Position& position, Move& move, int alpha, int beta)
{
	int StaticEval = EvaluatePosition(position);

	if (position.GetTurn() == BLACK)
	{
		if (StaticEval + 100 <= alpha) return true;
	}

	if (position.GetTurn() == WHITE)
	{
		if (StaticEval - 100 >= beta) return true;
	}

	return false;
}

void AlphaBeta(Position& position, int depth, ABnode* parent, int alpha, int beta, bool allowNull, SearchLevels search)
{
	if (timeManage.AbortSearch()) return;

	std::vector<Move> moves;
	bool InCheck = IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn());

	if (InitializeSearchVariables(position, moves, depth, alpha, beta, parent, search, InCheck)) return; //This checks for transpositions, draws, generates the moves, and checkmate

	//Null move prune
	if (!InCheck && allowNull && depth >= 4)
		if (NullMovePrune(position, depth, parent, alpha, beta, search)) return;

	ABnode* best = CreatePlaceHolderNode(position.GetTurn(), depth);

	bool Futile = true;

	for (int i = 0; i < moves.size(); i++)
	{
		position.ApplyMove(moves.at(i));

		//futility pruning. Dont prune promotions, captures or checks. Make sure it's more than 100 points below alpha (if so and previous condtitions met then prune)
		if (depth <= 2 && !InCheck && !IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()) && IsFutile(position, moves[i], alpha, beta)) {
			position.RevertMove(moves.at(i));
			continue;
		}
		Futile = false; //at least one move made it past the above line

		ABnode* node = CreateBranchNode(moves.at(i), depth);
		SearchLevels Newsearch = CalculateSearchType(position, depth, InCheck);
		bool Cut = false;

		//Late move reductions	
		if (i >= 4 && depth >= 4 && !moves.at(i).IsCapture() && !moves.at(i).IsPromotion() && !InCheck)
			if (LateMoveReduction(position, depth, node, alpha, beta, Newsearch))
				Cut = true;

		if (!Cut)
			if (Newsearch != ALPHABETA)
				Quietessence(position, depth - 1, node, alpha, beta, Newsearch);
			else
				AlphaBeta(position, depth - 1, node, alpha, beta, true, Newsearch);

		position.RevertMove(moves.at(i));

		SetBest(best, node, position.GetTurn());
		if (CheckForCutoff(alpha, beta, best, position.GetTurn() ? BETA_CUTOFF : ALPHA_CUTOFF)) break;
	}

	if (Futile)
	{	//every move was found to be futile
		best->SetScore(EvaluatePosition(position)); //this is safe because the score + a bonus was less than alpha, so we know theres a better line elsewhere. EDIT: possible zanzuag issue
	}

	parent->SetChild(best);
	tTable.AddEntry(best->GetMove(), position.GetZobristKey(), best->GetScore(), depth, best->GetCutoff());
}

bool NullMovePrune(Position& position, int depth, ABnode* parent, int alpha, int beta, SearchLevels search)
{
	if (position.GetTurn() == WHITE)
	{ //whites turn to move, which we are skipping to see if its still good for white 'beta cutoff'
		position.ApplyNullMove();
		AlphaBeta(position, depth - 3, parent, beta - 1, beta, false, search);
		position.RevertNullMove();
		return (parent->GetCutoff() == BETA_CUTOFF);
	}

	if (position.GetTurn() == BLACK)
	{
		position.ApplyNullMove();
		AlphaBeta(position, depth - 3, parent, alpha, alpha + 1, false, search);
		position.RevertNullMove();
		return (parent->GetCutoff() == ALPHA_CUTOFF);
	}

	return false;
}

bool LateMoveReduction(Position& position, int depth, ABnode* parent, int alpha, int beta, SearchLevels search)
{
	//int Eval = EvaluatePosition(position);

	if (position.GetTurn() == WHITE)
	{	//just did a black move, so we see if this position is good for white (hence check for beta cutoff)
		AlphaBeta(position, depth - 2, parent, beta - 1, beta, true, search);
		return (parent->GetCutoff() == BETA_CUTOFF);
	}

	if (position.GetTurn() == BLACK)
	{
		AlphaBeta(position, depth - 2, parent, alpha, alpha + 1, true, search);
		return (parent->GetCutoff() == ALPHA_CUTOFF);
	}

	return false;
}
