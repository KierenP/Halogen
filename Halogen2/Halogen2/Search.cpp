#include "Search.h"

TranspositionTable tTable;
ABnode Maximize(Position & position, int depth);
ABnode Minimize(Position & position, int depth);

/*int PositionTempName::NegaAlphaBetaMax(ABnode* parent, int depth, int alpha, int beta, int colour, bool AllowNull)
{
	int alphaOrig = alpha;
	NodeCount++;

	//1. Lookup Transposition table
	if (tTable.CheckEntry(GenerateZobristKey(), depth))
	{
		TTEntry ttEntry = tTable.GetEntry(GenerateZobristKey());
		parent->SetCutoff(ttEntry.GetCutoff());
		return ttEntry.GetScore();
	}

	//Futility pruning heuristic
	int staticEval = Evaluate();
	int PieceValues[N_PIECES + 1] = { 100, 300, 300, 500, 900, 20000, 100, 300, 300, 500, 900, 20000, 0 };

	//2. Null move heuristic
	if (AllowNull && staticEval >= beta)
	{
		bool MeInCheck = IsInCheck(GetKing(BoardParamiter.CurrentTurn), BoardParamiter.CurrentTurn);
		bool OtherInCheck = IsInCheck(GetKing(!BoardParamiter.CurrentTurn), !BoardParamiter.CurrentTurn);

		if (!MeInCheck && !OtherInCheck)
		{
			ApplyMove(Move(0, 0, NULL_MOVE));																	//Make null move
			ABnode* node = new ABnode(NONE, Move(), depth, -99999);
			int NullEval = -NegaAlphaBetaMax(node, depth - 3, -beta, -beta + 1, -colour, !AllowNull);			//narrow window because we want to see if we can still beat beta, we don't care how much lower it can go
			delete node;
			RevertMove(Move(0, 0, NULL_MOVE));																	//Revert null move
			if (NullEval > beta)																				//outside alpha / beta range hence cutoff
			{
				parent->SetCutoff(NULL_MOVE_PRUNE);
				return NullEval;
			}
		}
	}
	
	//Quiesence search terminator
	bool AllQuiet = true;

	//3. Search child nodes to find score
	std::vector<Move> moves = MoveOrder(GenerateLegalMoves(), depth);

	if (moves.size() == 0)
	{
		parent->SetCutoff(EXACT);
		if (IsInCheck())
			return colour * -9500;					//Got checkmated
		else
			return 0;								//stalemate
	}

	ABnode* best = new ABnode(NONE, Move(), depth, -99999);
	for (int i = 0; i < moves.size(); i++)
	{
		bool IsReCapture = (moves[i].GetTo() == BoardParamiter.CaptureSquare);
		bool IsCapture = moves[i].IsCapture();
		bool IsPromotion = moves[i].IsPromotion();
		bool MeInCheck = IsInCheck(GetKing(BoardParamiter.CurrentTurn), BoardParamiter.CurrentTurn);
		bool OtherInCheck = IsInCheck(GetKing(!BoardParamiter.CurrentTurn), !BoardParamiter.CurrentTurn);
		bool IsFutile = staticEval + PieceValues[GetSquare(moves[i].GetTo())] * 1.2 + 100 < alphaOrig;		//20% buffer

		if (depth <= 0 && !IsReCapture && !MeInCheck && !OtherInCheck && !IsPromotion)						//Quiesence search
			continue;
		if ((depth == 2 || depth == 1) && IsFutile && !(IsPromotion || MeInCheck || OtherInCheck))			//Futility pruning
			continue;

		AllQuiet = false;
		
		ApplyMove(moves[i]);
		ABnode* node = new ABnode(NONE, moves[i], depth);

		if (CheckThreeFold())		
			node->SetScore(0);																				//Draw with 3 fold rep
		else
			node->SetScore(-NegaAlphaBetaMax(node, depth - 1, -beta, -alpha, -colour, !AllowNull));
		RevertMove(moves[i]);
		
		if (node->GetScore() > best->GetScore())															//This move was better than any previous
		{
			delete best;
			best = node;
		}
		else
			delete node;

		alpha = fmax(alpha, best->GetScore());							
		if (alpha > beta)																					//outside alpha / beta range hence cutoff
		{
			if (colour == 1)
			{
				parent->SetCutoff(ALPHA_CUTOFF);
				parent->SetChild(best);
				best->SetParent(parent);
				return best->GetScore();
			}
			if (colour == -1)
			{
				parent->SetCutoff(BETA_CUTOFF);
				parent->SetChild(best);
				best->SetParent(parent);
				return best->GetScore();
			}
		}																			
	}

	if (AllQuiet)
	{
		delete best;
		parent->SetCutoff(EXACT);
		int score = colour * Evaluate();
		tTable.AddEntry(Move(), GenerateZobristKey(), score, depth, EXACT);
		return score;
	}

	//4. Add calculation to Transposition Table
	if (best->GetCutoff() == EXACT)
		tTable.AddEntry(best->GetMove(), GenerateZobristKey(), best->GetScore(), depth, EXACT);

	//5. Return result
	best->SetParent(parent);
	parent->SetChild(best);
	parent->SetCutoff(best->GetCutoff());
	return best->GetScore();
}*/

void OrderMoves(std::vector<Move>& moves, Position & position, int searchDepth)
{
	//TODO potentially good quiet moves are moved to the back. Maybe consider inserting rather and swapping? But quiet move ordering is poor anyways so this will probably not increase strength

	unsigned int swapIndex = 0;
	Move temp(0, 0, 0);
	int PieceValues[N_PIECES] = { 1, 3, 3, 5, 9, 10, 1, 3, 3, 5, 9, 10 };			//relative values
																					
	if (tTable.CheckEntry(GenerateZobristKey(position), searchDepth - 1))
	{
		Move bestprev = tTable.GetEntry(GenerateZobristKey(position)).GetMove();
		for (int i = 0; i < moves.size(); i++)
		{
			if (bestprev == moves[i])
			{
				temp = moves[i];
				moves[i] = moves[0];
				moves[0] = temp;
				swapIndex++;
				break;
			}
		}
	}

	//Move promotions to front
	for (int i = swapIndex; i < moves.size(); i++)
	{
		if (moves[i].IsPromotion())
		{
			temp = moves[i];
			moves[i] = moves[swapIndex];
			moves[swapIndex] = temp;
			swapIndex++;
		}
	}

	//order in terms of least valuable attacker, most valuable piece
	for (int i = swapIndex; i < moves.size(); i++)
	{
		for (int j = PieceValues[KING] - PieceValues[PAWN]; j >= PieceValues[PAWN] - PieceValues[KING]; j--)
		{
			if (moves[i].IsCapture() && (PieceValues[position.GetSquare(moves[i].GetTo())] - PieceValues[position.GetSquare(moves[i].GetFrom())] == j))
			{
				temp = moves[i];
				moves[i] = moves[swapIndex];
				moves[swapIndex] = temp;
				swapIndex++;
			}
		}
	}
}

ABnode SearchBestMove(Position & position, int depth, Move prevBest)
{
	tTable.ResetCount();
	std::vector<Move> moves = GenerateLegalMoves(position);

	//with only one move, we can very quickly just return this move as there is zero benifet to searching the tree below this node
	if (moves.size() == 1)										
		return ABnode(moves[0], depth, FORCED_MOVE, 0, nullptr);

	OrderMoves(moves, position, depth);
	ABnode best;
	ABnode node;

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
		int Score = 0;
		position.ApplyMove(moves[i]);

		if (position.GetTurn() == BLACK)
			node = Minimize(position, depth - 1);
		if (position.GetTurn() == WHITE)
			node = Maximize(position, depth - 1);
		position.RevertMove(moves[i]);

		if (best.GetCutoff() == UNINITIALIZED_NODE || (Score > best.GetScore()))
		{
			best = node;
		}
	}

	return best;
}

ABnode Maximize(Position & position, int depth)
{
	if (depth == 0) return ABnode(Move(), depth, EXACT, EvaluatePosition(position), nullptr);
	
	std::vector<Move> moves = GenerateLegalMoves(position);
	//OrderMoves(moves, position, depth);
	ABnode best;

	for (int i = 0; i < moves.size(); i++)
	{	
		position.ApplyMove(moves[i]);
		ABnode node = Minimize(position, depth - 1);
		position.RevertMove(moves[i]);

		if (best.GetCutoff() == UNINITIALIZED_NODE || (node.GetScore() > best.GetScore()))
		{
			best = node;
		}
	}

	return best;
}

ABnode Minimize(Position & position, int depth)
{
	if (depth == 0) return ABnode(Move(), depth, EXACT, EvaluatePosition(position), nullptr);

	std::vector<Move> moves = GenerateLegalMoves(position);
	//OrderMoves(moves, position, depth);
	ABnode best;

	for (int i = 0; i < moves.size(); i++)
	{
		position.ApplyMove(moves[i]);
		ABnode node = Maximize(position, depth - 1);
		position.RevertMove(moves[i]);

		if (best.GetCutoff() == UNINITIALIZED_NODE || (node.GetScore() < best.GetScore()))
		{
			best = node;
		}
	}

	return best;
}
