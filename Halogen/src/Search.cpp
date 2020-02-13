#include "Search.h"

unsigned int MAX_DEPTH = 100;

enum Score
{
	HighINF = 30000,
	LowINF = -30000,

	MateScore = -10000,
	Draw = 0
};

struct Killer
{
	Move move[2];
};

const unsigned int MaxDepth = 100;

TranspositionTable tTable;
SearchTimeManage timeManage;
std::vector<Killer> KillerMoves(MAX_DEPTH);					//2 moves indexed by distanceFromRoot
unsigned int HistoryMatrix[N_SQUARES][N_SQUARES];	//first index is from square and 2nd index is to square
std::vector<unsigned int> FutilityMargin = { 100, 150, 250 };

void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth, int distanceFromRoot, int alpha, int beta, int colour);
void PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, int score, int alpha, int beta, Position& position, Move move);
void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth, int distanceFromRoot, int alpha, int beta, int colour);
void PrintBestMove(Move& Best);
bool UseTransposition(TTEntry& entry, int distanceFromRoot, int alpha, int beta);
bool CheckForRep(Position& position);
bool LMR(std::vector<Move>& moves, int i, int beta, int alpha, bool InCheck, Position& position, int depth);
bool IsFutile(int beta, int alpha, std::vector<Move>& moves, int i, bool InCheck, Position& position);
bool AllowedNull(bool allowedNull, Position& position, int beta, int alpha, int depth);
bool IsPV(int beta, int alpha);
void AddScoreToTable(int Score, int alphaOriginal, Position& position, int depth, int distanceFromRoot, int beta, Move bestMove);
void UpdateBounds(TTEntry& entry, int& alpha, int& beta);
void UpdatePV(std::deque<Move>& pv, std::deque<Move>& line, Move move);
int TerminalScore(Position& position, int distanceFromRoot);
int extension(Position & position, Move & move, int alpha, int beta);
Move GetHashMove(Position& position, int depth);
Move GetHashMove(Position& position);
void AddKiller(Move move, int distanceFromRoot);
void AddHistory(Move& move, int depth);

SearchResult NegaScoutRoot(Position& position, int depth, int alpha, int beta, int colour, int distanceFromRoot, bool PrintCurrentMove);
SearchResult NegaScout(Position& position, int depth, int alpha, int beta, int colour, int distanceFromRoot, bool allowedNull);
SearchResult Quiescence(Position& position, int alpha, int beta, int colour, int distanceFromRoot, int depth);

/*
TODO list:
1. Adjust the criteria for how killer moves are stored
2. Consider putting underpromotions ahead of killer moves?


*/

std::vector<int> RelativePieceValues = { 100, 300, 300, 500, 900, 10000, 100, 300, 300, 500, 900, 10000 };	//technically, there is no way we can be capturing a king as the game would already be over.

int hits = 0;
int misses = 0;

void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth, int distanceFromRoot, int alpha, int beta, int colour)
{
	/*
	We want to order the moves such that the best moves are more likely to be further towards the front.

	The order is as follows:

	1. Hash move
	2. Winning captures and queen promotions (SEE > 0)
	3. Killer moves
	4. Quiet moves (further sorted by history matrix values)
	5. Losing captures

	*/

	Move TTmove = GetHashMove(position);

	/*Internal iterative deepening*/
	//basically, if we have no hash move, do a shallow search and make that the hash move

	if (TTmove.GetFlag() == UNINITIALIZED && searchDepth > 3)
	{
		TTmove = NegaScout(position, searchDepth - 2, alpha, beta, colour, distanceFromRoot, true).GetMove();
	}

	//give each move a score on how 'good' it is. 
	for (int i = 0; i < moves.size(); i++)
	{
		moves[i].SEE = 0;		//default to zero

		if (moves[i] == TTmove) {
			moves[i].SEE = 15000;	
			continue;
		}

		if (moves[i] == KillerMoves.at(distanceFromRoot).move[0])
			moves[i].SEE = 20;
		if (moves[i] == KillerMoves.at(distanceFromRoot).move[1])
			moves[i].SEE = 10;

		if (moves[i].IsCapture())
		{
			int PieceValue = RelativePieceValues.at(position.GetSquare(moves[i].GetFrom()));
			int CaptureValue = 0;

			if (moves[i].GetFlag() == EN_PASSANT)
				CaptureValue = RelativePieceValues[WHITE_PAWN];	

			if (moves[i].IsCapture() && moves[i].GetFlag() != EN_PASSANT)
			{
				CaptureValue = RelativePieceValues.at(position.GetSquare(moves[i].GetTo()));	//if enpassant then the .GetTo() square is empty
			}

			moves[i].SEE += CaptureValue;

			if (IsSquareThreatened(position, moves[i].GetTo(), position.GetTurn()) && !moves[i].IsPromotion())
				moves[i].SEE -= PieceValue;
		}

		if (moves[i].IsPromotion())
		{
			if (moves[i].GetFlag() == KNIGHT_PROMOTION || moves[i].GetFlag() == KNIGHT_PROMOTION_CAPTURE)
				moves[i].SEE = 1;
			if (moves[i].GetFlag() == ROOK_PROMOTION || moves[i].GetFlag() == ROOK_PROMOTION_CAPTURE)
				moves[i].SEE = 1;
			if (moves[i].GetFlag() == BISHOP_PROMOTION || moves[i].GetFlag() == BISHOP_PROMOTION_CAPTURE)
				moves[i].SEE = 1;
			if (moves[i].GetFlag() == QUEEN_PROMOTION || moves[i].GetFlag() == QUEEN_PROMOTION_CAPTURE)
				moves[i].SEE += RelativePieceValues[WHITE_QUEEN];
		}
	}

	std::stable_sort(moves.begin(), moves.end(), [](const Move& lhs, const Move& rhs)
		{
			if (lhs.SEE == 0 && rhs.SEE == 0)	//comparing quiet moves
				return HistoryMatrix[lhs.GetFrom()][lhs.GetTo()] > HistoryMatrix[rhs.GetFrom()][rhs.GetTo()];

			return lhs.SEE > rhs.SEE;
		});
}

void PrintBestMove(Move& Best)
{
	std::cout << "bestmove ";
	Best.Print();
	std::cout << std::endl;
}

void PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, int score, int alpha, int beta, Position& position, Move move)
{
	uint64_t actualNodeCount = position.GetNodeCount();

	std::vector<Move> pv;
	pv.push_back(move);
	position.ApplyMove(move);																					

	bool repeat = false;
	while (tTable.CheckEntry(position.GetZobristKey()) && !repeat && pv.size() < 100 && pv.size() / 2 < depth)	//see if there are any more moves in the pv we can extract from the transposition table
	{
		uint64_t current = position.GetZobristKey();
		for (int i = 0; i < position.GetPreviousKeysSize(); i++)
		{
			if (position.GetPreviousKey(i) == current)
				repeat = true;
		}

		pv.push_back(tTable.GetEntry(position.GetZobristKey()).GetMove());
		position.ApplyMove(pv.back());
	}

	for (int i = 0; i < pv.size(); i++)
	{
		position.RevertMove();
	}

	std::cout
		<< "info depth " << depth																				//the depth of search
		<< " seldepth " << pv.size();															//the selective depth (for example searching further for checks and captures)

	if (isCheckmate)
	{
		if (score > 0)
			std::cout << " score mate " << ((-abs(score) -MateScore) + 1) / 2;
		else
			std::cout << " score mate " << -((-abs(score) - MateScore) + 1) / 2;
	}
	else
	{
		std::cout << " score cp " << score;							//The score in hundreths of a pawn (a 1 pawn advantage is +100)	
	}

	if (score <= alpha)
		std::cout << " upperbound";
	if (score >= beta)
		std::cout << " lowerbound";

	std::cout
		<< " time " << Time																						//Time in ms
		<< " nodes " << actualNodeCount
		<< " nps " << int(actualNodeCount / max(Time, 1) * 1000)
		<< " hashfull " << unsigned int(float(tTable.GetCapacity()) / tTable.GetSize() * 1000)							//thousondths full
		<< " hashHitRate " << tTable.GetHitCount() * 1000 / max(actualNodeCount, 1)
		<< " pawnHitRate " << pawnHashTable.HashHits * 1000 / max(pawnHashTable.HashHits + pawnHashTable.HashMisses, 1)
		<< " pv ";																								//the current best line found



	for (int i = 0; i < pv.size(); i++)
	{
		pv[i].Print();
		std::cout << " ";
	}

	std::cout << std::endl;
}

Move SearchPosition(Position position, int allowedTimeMs)
{
	Move move;
	timeManage.StartSearch(allowedTimeMs);
	tTable.SetAllAncient();

	position.ResetNodeCount();
	tTable.ResetHitCount();

	Timer searchTime;
	searchTime.Start();

	int alpha = -30000;
	int beta = 30000;

	for (int i = 0; i < 64; i++)
	{
		for (int j = 0; j < 64; j++)
		{
			HistoryMatrix[i][j] = 0;
		}
	}

	for (int i = 0; i < KillerMoves.size(); i++)
	{
		KillerMoves.at(i).move[0] = Move();
		KillerMoves.at(i).move[1] = Move();
	}

	for (int depth = 1; !timeManage.AbortSearch(position.GetNodeCount()) && timeManage.ContinueSearch() && depth < 100; )
	{
		SearchResult search = NegaScoutRoot(position, depth, alpha, beta, position.GetTurn() ? 1 : -1, 0, searchTime.ElapsedMs() > 1000);
		int score = search.GetScore();
		Move returnMove = search.GetMove();

		if (timeManage.AbortSearch(position.GetNodeCount())) {	break; }

		if (score <= alpha || score >= beta)
		{
			//PrintSearchInfo(depth, searchTime.ElapsedMs(), abs(score) > 9000, score, alpha, beta, position, returnMove);
			alpha = -30000;
			beta = 30000;
			continue;
		}

		move = returnMove;	//this is only hit if the continue before is not hit
		PrintSearchInfo(depth, searchTime.ElapsedMs(), abs(score) > 9000, score, alpha, beta, position, move);

		depth++;
		alpha = score - 25;
		beta = score + 25;
	}

	PrintBestMove(move);
	return move;
}

SearchResult NegaScoutRoot(Position& position, int depth, int alpha, int beta, int colour, int distanceFromRoot, bool PrintCurrentMove)
{
	if (timeManage.AbortSearch(position.GetNodeCount())) return 0;

	std::vector<Move> moves;
	LegalMoves(position, moves);

	if (moves.size() == 0)
	{
		return TerminalScore(position, distanceFromRoot);
	}

	//mate distance pruning
	alpha = max(Score::MateScore + distanceFromRoot, alpha);
	beta = min(-Score::MateScore - distanceFromRoot, beta);
	if (alpha >= beta)
		return alpha;

	OrderMoves(moves, position, depth, distanceFromRoot, alpha, beta, colour);
	Move bestMove;
	int Score = LowINF;
	int a = alpha;
	int b = beta;

	for (int i = 0; i < moves.size() && !timeManage.AbortSearch(position.GetNodeCount()); i++)
	{
		if (PrintCurrentMove) {
			std::cout << "info currmovenumber " << i + 1 << " currmove ";
			moves[i].Print();
			std::cout << "\n";
		}

		position.ApplyMove(moves.at(i));

		int extendedDepth = depth + extension(position, moves[i], alpha, beta);
		int newScore = -NegaScout(position, extendedDepth - 1, -b, -a, -colour, distanceFromRoot + 1, true).GetScore();
		if (newScore > a && newScore < beta && i >= 1)
		{
			newScore = -NegaScout(position, extendedDepth - 1, -beta, -a, -colour, distanceFromRoot + 1, true).GetScore();	//possibly -newScore rather than -alpha
		}

		position.RevertMove();

		if (newScore > Score)
		{
			Score = newScore;
			bestMove = moves.at(i);
		}

		a = max(Score, a);
		if (a >= beta) //Fail high cutoff
		{
			AddKiller(moves.at(i), distanceFromRoot);
			AddHistory(moves[i], depth);
			break;
		}

		b = a + 1;				//Set a new zero width window
	}

	if (!timeManage.AbortSearch(position.GetNodeCount()))
		AddScoreToTable(Score, alpha, position, depth, distanceFromRoot, beta, bestMove);

	return SearchResult(Score, bestMove);
}

SearchResult NegaScout(Position& position, int depth, int alpha, int beta, int colour, int distanceFromRoot, bool allowedNull)
{
	if (timeManage.AbortSearch(position.GetNodeCount())) return -1;		//we must check later that we don't let this score pollute the transposition table
	if (distanceFromRoot >= MAX_DEPTH) return 0;			//If we are 100 moves from root I think we can assume its a drawn position

	//check for draw
	if (InsufficentMaterial(position)) return 0;
	if (CheckForRep(position)) return 0;

	/*Query the transpotition table*/
	if (tTable.CheckEntry(position.GetZobristKey(), depth))
	{
		TTEntry entry = tTable.GetEntry(position.GetZobristKey());
		if (UseTransposition(entry, distanceFromRoot, alpha, beta)) return SearchResult(entry.GetScore(), entry.GetMove());
	}

	/*Drop into quiescence search*/
	if (depth <= 0 && !IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn()))
	{ 
		return Quiescence(position, alpha, beta, colour, distanceFromRoot, depth);
	}

	std::vector<Move> moves;
	LegalMoves(position, moves);

	if (moves.size() == 0)
	{
		return TerminalScore(position, distanceFromRoot);
	}

	/*Null move pruning*/
	if (AllowedNull(allowedNull, position, beta, alpha, depth))
	{
		position.ApplyNullMove();
		int score = -NegaScout(position, depth - 3, -beta, -beta + 1, -colour, distanceFromRoot + 1, false).GetScore();	
		position.RevertNullMove();

		//Verification search
		if (score >= beta)
		{	
			//why true? I can't justify but analysis improved in WAC to 297/300. Possibly recursive null calls produces rare speedups where significant work is skipped
			SearchResult result = NegaScout(position, depth - 3, beta - 1, beta, colour, distanceFromRoot, true);

			if (result.GetScore() >= beta)
				return result;
		}

		
	}

	//mate distance pruning
	alpha = max(Score::MateScore + distanceFromRoot, alpha);
	beta = min(-Score::MateScore - distanceFromRoot, beta);
	if (alpha >= beta)
		return alpha;

	OrderMoves(moves, position, depth, distanceFromRoot, alpha, beta, colour);
	Move bestMove = Move();	//used for adding to transposition table later
	int Score = LowINF;
	int a = alpha;
	int b = beta;
	bool InCheck = IsInCheck(position);
	int staticScore = colour * EvaluatePosition(position);
	bool futile = false;

	/*Futility pruning*/
	if (depth < FutilityMargin.size() && staticScore + FutilityMargin.at(max(0, depth)) < alpha && !IsPV(beta, alpha) && !InCheck && !(alpha < -9000 || beta > 9000))
		futile = true;

	for (int i = 0; i < moves.size(); i++)	
	{
		position.ApplyMove(moves.at(i));
		tTable.PreFetch(position.GetZobristKey());							//load the transposition into l1 cache. ~5% speedup

		//futility pruning
		if (futile && !moves[i].IsCapture() && !moves[i].IsPromotion() && !IsInCheck(position) && i > 0)	//Possibly stop futility pruning if alpha or beta are close to mate scores. Note the turn has changed and IsInCheck is not the same as `bool InCheck` above
		{
			position.RevertMove();
			continue;	
		}

		int extendedDepth = depth + extension(position, moves[i], alpha, beta);

		//late move reductions
		if (LMR(moves, i, beta, alpha, InCheck, position, depth) && i > 3)
		{
			int score = -NegaScout(position, depth - 2, -a - 1, -a, -colour, distanceFromRoot + 1, true).GetScore();

			if (score < a)
			{
				position.RevertMove();
				continue;
			}
		}

		int newScore = -NegaScout(position, extendedDepth - 1, -b, -a, -colour, distanceFromRoot + 1, true).GetScore();
		if (newScore > a && newScore < beta && i >= 1)
		{	
			newScore = -NegaScout(position, extendedDepth - 1, -beta, -alpha, -colour, distanceFromRoot + 1, true).GetScore();
		}

		position.RevertMove();

		if (newScore > Score)
		{
			Score = newScore;
			bestMove = moves.at(i);
		}

		a = max(Score, a);
		if (a >= beta) //Fail high cutoff
		{
			AddKiller(moves.at(i), distanceFromRoot);
			AddHistory(moves[i], depth);
			break;
		}

		b = a + 1;				//Set a new zero width window
	}

	if (!timeManage.AbortSearch(position.GetNodeCount()))
		AddScoreToTable(Score, alpha, position, depth, distanceFromRoot, beta, bestMove);

	return SearchResult(Score, bestMove);
}

bool UseTransposition(TTEntry& entry, int distanceFromRoot, int alpha, int beta)
{
	tTable.AddHit();
	entry.MateScoreAdjustment(distanceFromRoot);	//this MUST be done

	if (entry.GetCutoff() == EntryType::EXACT) return true;

	int NewAlpha = alpha;
	int NewBeta = beta;

	UpdateBounds(entry, NewAlpha, NewBeta);	//aspiration windows and search instability lead to issues with shrinking the original window

	if (NewAlpha >= NewBeta)
		return true;

	return false;
}

bool CheckForRep(Position& position)
{
	int rep = 1;
	uint64_t current = position.GetZobristKey();

	//note Previous keys will not contain the current key, hence rep starts at one
	for (int i = 0; i < position.GetPreviousKeysSize(); i++)
	{
		if (position.GetPreviousKey(i) == current)
			rep++;

		if (rep == 2) return true;	//don't allow 2 fold rep instead of permitting it and then disallowing 3 fold rep
	}
	
	return false;
}

int extension(Position & position, Move & move, int alpha, int beta)
{
	int extension = 0;

	if (IsPV(beta, alpha))
	{
		if (IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn()))	
			extension += 1;

		if (position.GetSquare(move.GetTo()) == WHITE_PAWN && GetRank(move.GetTo()) == RANK_7)	//note the move has already been applied
			extension += 1;

		if (position.GetSquare(move.GetTo()) == BLACK_PAWN && GetRank(move.GetTo()) == RANK_2)
			extension += 1;
	}
	else
	{
		if (IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn()) && move.SEE >= 0)
			extension += 1;

		if (position.GetSquare(move.GetTo()) == WHITE_PAWN && GetRank(move.GetTo()) == RANK_7)	//note the move has already been applied
			extension += 1;

		if (position.GetSquare(move.GetTo()) == BLACK_PAWN && GetRank(move.GetTo()) == RANK_2)
			extension += 1;
	}

	return extension;
}

bool LMR(std::vector<Move>& moves, int i, int beta, int alpha, bool InCheck, Position& position, int depth)
{
	return !moves[i].IsCapture()
		&& !moves[i].IsPromotion()
		&& !IsPV(beta, alpha)
		&& !InCheck 
		&& !IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn()) 
		&& depth > 3;
}

bool IsFutile(int beta, int alpha, std::vector<Move>& moves, int i, bool InCheck, Position& position)
{
	return !IsPV(beta, alpha)
		&& !moves[i].IsCapture() 
		&& !moves[i].IsPromotion() 
		&& !InCheck 
		&& !IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn());
}

bool AllowedNull(bool allowedNull, Position& position, int beta, int alpha, int depth)
{
	return allowedNull
		&& !IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn())
		&& !IsPV(beta, alpha)
		&& depth > 3									//don't drop directly into quiessence search. particularly important in mate searches as quiessence search has no mate detection currently. See 5rk1/2p4p/2p4r/3P4/4p1b1/1Q2NqPp/PP3P1K/R4R2 b - - 0 1 
		&& GetBitCount(position.GetAllPieces()) >= 5;	//avoid null move pruning in very late game positions due to zanauag issues. Even with verification search e.g 8/6k1/8/8/8/8/1K6/Q7 w - - 0 1 
}

bool IsPV(int beta, int alpha)
{
	return beta != alpha + 1;
}

void AddScoreToTable(int Score, int alphaOriginal, Position& position, int depth, int distanceFromRoot, int beta, Move bestMove)
{
	if (Score <= alphaOriginal)
		tTable.AddEntry(bestMove, position.GetZobristKey(), Score, depth, distanceFromRoot, EntryType::UPPERBOUND);	//mate score adjustent is done inside this function
	else if (Score >= beta)
		tTable.AddEntry(bestMove, position.GetZobristKey(), Score, depth, distanceFromRoot, EntryType::LOWERBOUND);
	else
		tTable.AddEntry(bestMove, position.GetZobristKey(), Score, depth, distanceFromRoot, EntryType::EXACT);
}

void UpdateBounds(TTEntry& entry, int& alpha, int& beta)
{
	if (entry.GetCutoff() == EntryType::LOWERBOUND)
	{
		alpha = max(alpha, entry.GetScore());
	}

	if (entry.GetCutoff() == EntryType::UPPERBOUND)
	{
		beta = min(beta, entry.GetScore());
	}
}

void UpdatePV(std::deque<Move>& pv, std::deque<Move>& line, Move move)
{
	pv = line;
	pv.push_front(move);
}

int TerminalScore(Position& position, int distanceFromRoot)
{
	if (IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn()))
	{
		return (MateScore) + (distanceFromRoot);
	}
	else
	{
		return (Draw);
	}
}

SearchResult Quiescence(Position& position, int alpha, int beta, int colour, int distanceFromRoot, int depth)
{
	if (timeManage.AbortSearch(position.GetNodeCount())) return -1;
	if (distanceFromRoot >= MAX_DEPTH) return 0;			//If we are 100 moves from root I think we can assume its a drawn position

	std::vector<Move> moves;

	/*Check for checkmate*/
	if (IsInCheck(position))
	{
		LegalMoves(position, moves);

		if (moves.size() == 0)
		{
			return TerminalScore(position, distanceFromRoot);
		}

		moves.clear();
	}

	int staticScore = colour * EvaluatePosition(position);
	if (staticScore >= beta) return staticScore;
	if (staticScore > alpha) alpha = staticScore;

	
	Move bestmove;
	int Score = staticScore;

	QuiescenceMoves(position, moves);

	if (moves.size() == 0)
		return staticScore;
		
	OrderMoves(moves, position, depth, distanceFromRoot, alpha, beta, colour);

	for (int i = 0; i < moves.size(); i++)
	{
		if (staticScore + moves[i].SEE + 200 < alpha) 								//delta pruning
			break;

		if (moves[i].SEE < 0)														//prune bad captures
			break;

		if (moves[i].SEE <= 0 && position.GetCaptureSquare() != moves[i].GetTo())	//prune equal captures that aren't recaptures
			continue;

		if (moves[i].IsPromotion() && !(moves[i].GetFlag() == QUEEN_PROMOTION || moves[i].GetFlag() == QUEEN_PROMOTION_CAPTURE))	//prune underpromotions
			continue;

		position.ApplyMove(moves.at(i));
		int newScore = -Quiescence(position, -beta, -alpha, -colour, distanceFromRoot + 1, depth - 1).GetScore();
		position.RevertMove();

		if (newScore > Score)
		{
			bestmove = moves.at(i);
			Score = newScore;
		}

		alpha = max(Score, alpha);
		if (Score >= beta)
			break;
	}

	return SearchResult(Score, bestmove);
}

void AddKiller(Move move, int distanceFromRoot)
{
	if (move.IsCapture() || move.IsPromotion()) return;

	if (move == KillerMoves.at(distanceFromRoot).move[0]) return;
	if (move == KillerMoves.at(distanceFromRoot).move[1])
	{
		//swap to the front
		Move temp = KillerMoves.at(distanceFromRoot).move[0];
		KillerMoves.at(distanceFromRoot).move[0] = KillerMoves.at(distanceFromRoot).move[1];
		KillerMoves.at(distanceFromRoot).move[1] = temp;

		return;
	}

	KillerMoves.at(distanceFromRoot).move[1] = move;	//replace the 2nd one
}

void AddHistory(Move& move, int depth)
{
	HistoryMatrix[move.GetFrom()][move.GetTo()] += depth * depth;
}

Move GetHashMove(Position& position, int depth)
{
	if (tTable.CheckEntry(position.GetZobristKey(), depth))
	{
		return tTable.GetEntry(position.GetZobristKey()).GetMove();
	}

	return {};
}

Move GetHashMove(Position& position)
{
	if (tTable.CheckEntry(position.GetZobristKey()))
	{
		return tTable.GetEntry(position.GetZobristKey()).GetMove();
	}

	return {};
}
