#include "Position.h"


int Position::NegaAlphaBetaMax(ABnode* parent, int depth, int alpha, int beta, int colour, bool AllowNull)
{
	int alphaOrig = alpha;
	NodeCount++;

	//1. Lookup Transposition table
	if (tTable.CheckEntry(GenerateZobristKey(), depth))
	{
		TTEntry ttEntry = tTable.GetEntry(GenerateZobristKey());
		if (ttEntry.GetCutoff() == EXACT || ttEntry.GetCutoff() == ALPHA_CUTOFF)
		{
			parent->SetCutoff(ttEntry.GetCutoff());
			return ttEntry.GetScore();
		}
	}

	//2. Null move heuristic
	if (AllowNull)
	{
		bool MeInCheck = IsInCheck(GetKing(BoardParamiter.CurrentTurn), BoardParamiter.CurrentTurn);
		bool OtherInCheck = IsInCheck(GetKing(!BoardParamiter.CurrentTurn), !BoardParamiter.CurrentTurn);

		if (!MeInCheck)
		{
			bool turn = BoardParamiter.CurrentTurn;
			BoardParamiter.CurrentTurn = !turn;		//make null move
			ABnode* node = new ABnode(NONE, Move(), depth, -99999);
			int NullEval = -NegaAlphaBetaMax(node, depth - 3, -beta, -beta + 1, -colour, !AllowNull);
			delete node;
			BoardParamiter.CurrentTurn = turn;		//Unmake null move
			if (NullEval > beta)
			{
				parent->SetCutoff(BETA_CUTOFF);
				return beta;
			}
		}
	}

	//Futility pruning heuristic
	int PieceValues[N_PIECES + 1] = { 100, 300, 300, 500, 900, 20000, 100, 300, 300, 500, 900, 20000, 0 };	
	
	//Quiesence search terminator
	bool AllQuiet = true;

	//3. Search child nodes to find score
	ABnode* best = new ABnode(NONE, Move(), depth, -99999);
	std::vector<Move> moves = MoveOrder(GenerateLegalMoves(), depth);

	if (moves.size() == 0)
	{
		delete best;
		parent->SetCutoff(EXACT);
		if (IsInCheck())
			return colour * -9500;					//Got checkmated
		else
			return 0;								//stalemate
	}

	int staticEval = Evaluate();

	for (int i = 0; i < moves.size(); i++)
	{
		bool IsReCapture = (moves[i].GetTo() == PreviousParam[BoardParamiter.TurnCount - 1].CaptureSquare);
		bool IsCapture = moves[i].IsCapture();
		bool IsPromotion = moves[i].IsPromotion();
		bool MeInCheck = IsInCheck(GetKing(BoardParamiter.CurrentTurn), BoardParamiter.CurrentTurn);
		bool OtherInCheck = IsInCheck(GetKing(!BoardParamiter.CurrentTurn), !BoardParamiter.CurrentTurn);
		bool IsFutile = staticEval + PieceValues[GetSquare(moves[i].GetTo())] * 1.2 < alpha;				//20% buffer

		if (depth <= 0 && !IsReCapture && !MeInCheck && !OtherInCheck && !IsPromotion)						//Quiesence search
			continue;
		if ((depth == 2 || depth == 1) && IsFutile && !(IsPromotion || MeInCheck || OtherInCheck))			//Futility pruning
			continue;

		AllQuiet = false;
		
		ApplyMove(moves[i]);
		ABnode* node = new ABnode(NONE, moves[i], depth);
		node->SetScore(-NegaAlphaBetaMax(node, depth - 1, -beta, -alpha, -colour, !AllowNull));
		RevertMove(moves[i]);
		
		if (node->GetScore() > best->GetScore())
		{
			delete best;
			best = node;
		}
		else
			delete node;

		alpha = fmax(alpha, best->GetScore());							//best score is either what it was last time, or it is now higher as the current node just replaced best.
		if (alpha >= beta)
			break;														//outside alpha / beta range hence cutoff
	}

	if (AllQuiet)
	{
		delete best;
		parent->SetCutoff(EXACT);
		return colour * Evaluate();
	}

	//4. Add calculation to Transposition Table
	if (best->GetScore() < alphaOrig)
		best->SetCutoff(ALPHA_CUTOFF);
	else if (best->GetScore() >= beta)
		best->SetCutoff(BETA_CUTOFF);
	else
		best->SetCutoff(EXACT);
	tTable.AddEntry(best->GetMove(), GenerateZobristKey(), best->GetScore(), depth, best->GetCutoff());

	//5. Return result
	best->SetParent(parent);
	parent->SetChild(best);
	parent->SetCutoff(best->GetCutoff());
	return best->GetScore();
}

/*
int Position::alphabetaMax(int alpha, int beta, int depth, ABnode* parent)
{
	if (tTable.CheckEntry(GenerateZobristKey(), depth))	//Check the TT for if we have already analysed this position
	{
		TTEntry entry = tTable.GetEntry(GenerateZobristKey());
		parent->SetCutoff(entry.GetCutoff());
		return entry.GetScore();
	}

	GeneratePsudoLegalMoves();

	MoveOrder(LegalMoves, depth);
	ABnode* best = new ABnode(ALPHA_CUTOFF, Move(0, 0, 0), depth, alpha);
	std::vector<Move> moves = LegalMoves;			//Make a copy of the legal moves for this position

	bool AllIllegal = true;
	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* node = new ABnode(NONE, moves[i], depth);

		ApplyMove(moves[i]);
		if (IsInCheck(GetKing(!BoardParamiter.CurrentTurn), !BoardParamiter.CurrentTurn))
		{
			RevertMove(moves[i]);
			continue;					//Illegal move
		}
		AllIllegal = false;

		if (!CheckThreeFold())
		{
			if (depth > 0 && !(depth < 2 && !IsInCheck() && !moves[i].IsCapture()))
				node->SetScore(alphabetaMin(best->GetScore(), beta, depth - 1, node));
			else
				node->SetScore(QuialphabetaMin(best->GetScore(), beta, depth - 1, node));
		}
		else
			node->SetScore(0);
		RevertMove(moves[i]);

		if (node->GetScore() >= beta)
		{
			parent->SetChild(node);
			parent->SetCutoff(BETA_CUTOFF);
			delete best;
			return beta;
		}

		if (node->GetScore() > best->GetScore())
		{
			delete best;
			best = node;
		}
		else
			delete node;
	}

	if (AllIllegal)
	{
		parent->SetCutoff(EXACT);
		if (IsInCheck())
			return CheckMateScore();
		else
			return 0;									//stalemate
	}

	parent->SetChild(best);
	parent->SetCutoff(best->GetCutoff());
	best->SetParent(parent);
	tTable.AddEntry(best->GetMove(), GenerateZobristKey(), best->GetScore(), depth, best->GetCutoff());
	return best->GetScore();
}

int Position::alphabetaMin(int alpha, int beta, int depth, ABnode* parent)
{
	if (tTable.CheckEntry(GenerateZobristKey(), depth))	//Check the TT for if we have already analysed this position
	{
		TTEntry entry = tTable.GetEntry(GenerateZobristKey());
		parent->SetCutoff(entry.GetCutoff());
		return entry.GetScore();
	}

	GeneratePsudoLegalMoves();

	MoveOrder(LegalMoves, depth);
	ABnode* best = new ABnode(BETA_CUTOFF, Move(0, 0, 0), depth, beta);
	std::vector<Move> moves = LegalMoves;

	bool AllIllegal = true;
	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* node = new ABnode(NONE, moves[i], depth);

		ApplyMove(moves[i]);
		if (IsInCheck(GetKing(!BoardParamiter.CurrentTurn), !BoardParamiter.CurrentTurn))
		{
			RevertMove(moves[i]);
			continue;					//Illegal move
		}
		AllIllegal = false;

		if (!CheckThreeFold())
		{
			if (depth > 0 && !(depth < 2 && !IsInCheck() && !moves[i].IsCapture()))
				node->SetScore(alphabetaMax(alpha, best->GetScore(), depth - 1, node));
			else
				node->SetScore(QuialphabetaMax(alpha, best->GetScore(), depth - 1, node));
		}
		else
			node->SetScore(0);
		RevertMove(moves[i]);

		if (node->GetScore() <= alpha)
		{
			parent->SetChild(node);
			parent->SetCutoff(ALPHA_CUTOFF);
			delete best;
			return alpha;
		}
		if (node->GetScore() < best->GetScore())
		{
			delete best;
			best = node;
		}
		else
			delete node;
	}

	if (AllIllegal)
	{
		parent->SetCutoff(EXACT);
		if (IsInCheck())
			return CheckMateScore();
		else
			return 0;									//stalemate
	}

	parent->SetChild(best);
	parent->SetCutoff(best->GetCutoff());
	best->SetParent(parent);
	tTable.AddEntry(best->GetMove(), GenerateZobristKey(), best->GetScore(), depth, best->GetCutoff());
	return best->GetScore();
}

int Position::QuialphabetaMax(int alpha, int beta, int depth, ABnode* parent)
{
	if (tTable.CheckEntry(GenerateZobristKey(), depth))	//Check the TT for if we have already analysed this position
	{
		TTEntry entry = tTable.GetEntry(GenerateZobristKey());
		parent->SetCutoff(entry.GetCutoff());
		return entry.GetScore();
	}

	GenerateLegalMoves();

	if (LegalMoves.size() == 0)
	{
		parent->SetCutoff(EXACT);
		if (IsInCheck())
			return CheckMateScore();
		else
			return 0;									//stalemate
	}

	MoveOrder(LegalMoves, depth);
	ABnode* best = new ABnode(ALPHA_CUTOFF, Move(0, 0, 0), depth, alpha);
	std::vector<Move> moves = LegalMoves;

	bool AllQuiet = true;
	for (int i = 0; i < moves.size(); i++)
	{
		if ((moves[i].GetTo() == BoardParamiter.CaptureSquare) || (IsInCheck(GetKing(BoardParamiter.CurrentTurn), BoardParamiter.CurrentTurn)) || (moves[i].IsPromotion()))
		{
			AllQuiet = false;
			ABnode* node = new ABnode(NONE, moves[i], depth);

			ApplyMove(moves[i]);
			if (!CheckThreeFold())
				node->SetScore(QuialphabetaMin(best->GetScore(), beta, depth - 1, node));
			else
				node->SetScore(0);
			RevertMove(moves[i]);

			if (node->GetScore() >= beta)
			{
				parent->SetChild(node);
				parent->SetCutoff(BETA_CUTOFF);
				delete best;
				return beta;
			}
			if (node->GetScore() > best->GetScore())
			{
				delete best;
				best = node;
			}
			else
				delete node;
		}
	}

	if (AllQuiet)
	{
		delete best;
		parent->SetCutoff(EXACT);
		return Evaluate();
	}

	parent->SetChild(best);
	parent->SetCutoff(best->GetCutoff());
	best->SetParent(parent);
	tTable.AddEntry(best->GetMove(), GenerateZobristKey(), best->GetScore(), depth, best->GetCutoff());
	return best->GetScore();
}

int Position::QuialphabetaMin(int alpha, int beta, int depth, ABnode* parent)
{
	if (tTable.CheckEntry(GenerateZobristKey(), depth))	//Check the TT for if we have already analysed this position
	{
		TTEntry entry = tTable.GetEntry(GenerateZobristKey());
		parent->SetCutoff(entry.GetCutoff());
		return entry.GetScore();
	}

	GenerateLegalMoves();

	if (LegalMoves.size() == 0)
	{
		parent->SetCutoff(EXACT);
		if (IsInCheck())
			return CheckMateScore();
		else
			return 0;									//stalemate
	}

	MoveOrder(LegalMoves, depth);
	ABnode* best = new ABnode(BETA_CUTOFF, Move(0, 0, 0), depth, beta);
	std::vector<Move> moves = LegalMoves;

	bool AllQuiet = true;
	for (int i = 0; i < moves.size(); i++)
	{
		if ((moves[i].GetTo() == BoardParamiter.CaptureSquare) || (IsInCheck(GetKing(BoardParamiter.CurrentTurn), BoardParamiter.CurrentTurn)) || (moves[i].IsPromotion()))
		{
			AllQuiet = false;
			ABnode* node = new ABnode(NONE, moves[i], depth);

			ApplyMove(moves[i]);
			if (!CheckThreeFold())
				node->SetScore(QuialphabetaMax(alpha, best->GetScore(), depth - 1, node));
			else
				node->SetScore(0);
			RevertMove(moves[i]);

			if (node->GetScore() <= alpha)
			{
				parent->SetChild(node);
				parent->SetCutoff(ALPHA_CUTOFF);
				delete best;
				return alpha;
			}
			if (node->GetScore() < best->GetScore())
			{
				delete best;
				best = node;
			}
			else
				delete node;
		}
	}

	if (AllQuiet)
	{
		delete best;
		parent->SetCutoff(EXACT);
		return Evaluate();
	}

	parent->SetChild(best);
	parent->SetCutoff(best->GetCutoff());
	best->SetParent(parent);
	tTable.AddEntry(best->GetMove(), GenerateZobristKey(), best->GetScore(), depth, best->GetCutoff());
	return best->GetScore();
}*/
