#include "Search.h"

enum Score
{
	HighINF = 30000,
	LowINF = -30000,

	MateScore = -10000,
	Draw = 0
};

const unsigned int MaxDepth = 100;
const unsigned int SearchIncrement = 2;

TranspositionTable tTable;
SearchTimeManage timeManage;
std::vector<Move[2]> KillerMoves(MaxDepth);	//2 moves indexed by distanceFromRoot

void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth, int distanceFromRoot);
void PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, std::deque<Move> pv, int score, Position& position);
void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth, int distanceFromRoot);
void PrintBestMove(Move& Best);
int NegaScout(Position& position, int depth, int alpha, int beta, int colour, int distanceFromRoot, bool allowedNull, std::deque<Move>& pv);
bool LMR(std::vector<Move>& moves, int i, int beta, int alpha, bool InCheck, Position& position, int depth);
bool IsFutile(int beta, int alpha, std::vector<Move>& moves, int i, bool InCheck, Position& position);
bool AllowedNull(bool allowedNull, Position& position, int beta, int alpha);
bool IsPV(int beta, int alpha);
void AddScoreToTable(int Score, int alphaOriginal, Position& position, int depth, int distanceFromRoot, int beta, Move bestMove);
void UpdateBounds(TTEntry& entry, int& alpha, int& beta);
void UpdatePV(std::deque<Move>& pv, std::deque<Move>& line, Move move);
int TerminalScore(Position& position, int distanceFromRoot);
int Quiescence(Position& position, int alpha, int beta, int colour, int distanceFromRoot, std::deque<Move>& pv);
int extension(Position & position, Move & move, int alpha, int beta);
Move GetHashMove(Position& position, int depth);
void AddKiller(Move move, int distanceFromRoot);

void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth, int distanceFromRoot)
{
	/*
	We want to order the moves such that the best moves are more likely to be further towards the front.
	*/

	Move TTmove = GetHashMove(position, searchDepth - SearchIncrement);
	std::vector<int> RelativePieceValues = { 100, 300, 300, 500, 900, 10000, 100, 300, 300, 500, 900, 10000 };	//technically, there is no way we can be capturing a king as the game would already be over.

	//give each move a score on how 'good' it is. 
	for (int i = 0; i < moves.size(); i++)
	{
		if (moves[i] == TTmove) {
			moves[i].SEE = 15000;	
			continue;
		}

		if (moves[i] == KillerMoves.at(distanceFromRoot)[0])
			moves[i].SEE = 20;
		if (moves[i] == KillerMoves.at(distanceFromRoot)[1])
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

			if (IsInCheck(position, moves[i].GetTo(), position.GetTurn()) && !moves[i].IsPromotion())
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
			return lhs.SEE > rhs.SEE;
		});
}

void PrintBestMove(Move& Best)
{
	std::cout << "bestmove ";
	Best.Print();
	std::cout << std::endl;
}

void PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, std::deque<Move> pv, int score, Position& position)
{
	for (int i = 0; i < pv.size(); i++)
	{
		position.ApplyMove(pv[i]);
	}

	bool repeat = false;
	while (tTable.CheckEntry(position.GetZobristKey()) && !repeat && pv.size() < 100)	//see if there are any more moves in the pv we can extract from the transposition table
	{
		uint64_t current = position.GetZobristKey();
		for (int i = 0; i < PreviousKeys.size(); i++)
		{
			if (PreviousKeys.at(i) == current)
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
		std::cout << " score mate " << (-abs(score) -MateScore) / 2;
	}
	else
		std::cout << " score cp " << score;							//The score in hundreths of a pawn (a 1 pawn advantage is +100)

	std::cout
		<< " time " << Time																						//Time in ms
		<< " nodes " << NodeCount
		<< " nps " << int(NodeCount / (Time) * 1000)
		<< " tbhits " << tTable.GetHitCount()
		<< " hashfull " << unsigned int(float(tTable.GetCapacity()) / TTSize * 1000)							//thousondths full
		<< " pv ";																								//the current best line found

	for (int i = 0; i < pv.size(); i++)
	{
		pv[i].Print();
		std::cout << " ";
	}

	std::cout << std::endl;
}

Move NegaMaxRoot(Position position, int allowedTimeMs)
{
	Move move;
	timeManage.StartSearch(allowedTimeMs);
	tTable.SetAllAncient();

	NodeCount = 0;
	tTable.ResetHitCount();

	Timer searchTime;
	searchTime.Start();

	int alpha = -30000;
	int beta = 30000;

	for (int depth = 1; !timeManage.AbortSearch() && timeManage.ContinueSearch() && depth < 100; )
	{
		std::deque<Move> pv;
		int score = NegaScout(position, depth * SearchIncrement, alpha, beta, position.GetTurn() ? 1 : -1, 1, true, pv);

		if (timeManage.AbortSearch()) {	break; }

		if (score <= alpha || score >= beta)
		{
			alpha = -30000;
			beta = 30000;
			std::cout << "info re-search\n";
			continue;
		}

		move = pv[0];	//this is only hit if the continue before is not hit
		PrintSearchInfo(depth, searchTime.ElapsedMs(), abs(score) > 9000, pv, score, position);

		depth++;			
		alpha = score - 25;
		beta = score + 25;
	}

	PrintBestMove(move);
	return move;
}

int NegaScout(Position& position, int depth, int alpha, int beta, int colour, int distanceFromRoot, bool allowedNull, std::deque<Move>& pv)
{
	pv.clear();
	if (timeManage.AbortSearch()) return 0;	

	//check for draw
	int rep = 1;
	uint64_t current = position.GetZobristKey();

	//note Previous keys will not contain the current key, hence rep starts at one
	for (int i = 0; i < PreviousKeys.size(); i++)
	{
		if (PreviousKeys.at(i) == current)
			rep++;

		if (rep == 2 && distanceFromRoot > 1) return 0;	//don't allow 2 fold rep, unless we are at the root in which case we are permitted it
		if (rep == 3) return 0;
	}

	/*Query the transpotition table*/
	if (tTable.CheckEntry(position.GetZobristKey(), depth))
	{
		tTable.AddHit();

		TTEntry entry = tTable.GetEntry(position.GetZobristKey());
		entry.MateScoreAdjustment(distanceFromRoot);	//this MUST be done

		if (distanceFromRoot == 1)
			pv.push_front(entry.GetMove());	//this is important as if we hit the table at the root we need to return this. We don't return it normally because this slows the program down

		if (entry.GetCutoff() == EntryType::EXACT) return entry.GetScore();

		int NewAlpha = alpha;
		int NewBeta = beta;

		UpdateBounds(entry, NewAlpha, NewBeta);	//aspiration windows and search instability lead to issues with shrinking the original window

		if (NewAlpha >= NewBeta)
			return entry.GetScore();
	}

	/*Drop into quiescence search*/
	std::deque<Move> line;
	if (depth <= 0 && !IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()))
	{ 
		int score = Quiescence(position, alpha, beta, colour, distanceFromRoot, line);
		pv = line;
		return score;
	}

	std::vector<Move> moves;
	LegalMoves(position, moves);

	if (moves.size() == 0)
	{
		return TerminalScore(position, distanceFromRoot);
	}

	if (AllowedNull(allowedNull, position, beta, alpha))
	{
		position.ApplyNullMove();
		int score = -NegaScout(position, depth - 3 * SearchIncrement, -beta, -beta + 1, -colour, distanceFromRoot + 2, false, line);
		position.RevertNullMove();

		if (score >= beta)
		{
			return score;
		}
	}

	//mate distance pruning
	alpha = max(Score::MateScore + distanceFromRoot, alpha);
	beta = min(-Score::MateScore - distanceFromRoot, beta);
	if (alpha >= beta)
		return alpha;

	OrderMoves(moves, position, depth, distanceFromRoot);
	Move bestMove = Move();	//used for adding to transposition table later
	Move hashMove = GetHashMove(position, depth);
	int Score = LowINF;
	int a = alpha;
	int b = beta;
	bool InCheck = IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn());
	int staticScore = colour * EvaluatePosition(position);

	for (int i = 0; i < moves.size(); i++)	
	{
		position.ApplyMove(moves.at(i));

		//futility pruning
		if (IsFutile(beta, alpha, moves, i, InCheck, position) && i > 0)	//note .GetTurn() has changed
		{
			if (staticScore + 200 < a && depth == 2)	
			{
				position.RevertMove();
				continue;	
			}
		}

		//late move reductions
		if (LMR(moves, i, beta, alpha, InCheck, position, depth) && i > 3)
		{
			int score = -NegaScout(position, depth - 2 * SearchIncrement, -a - 1, -a, -colour, distanceFromRoot + 1, true, line);

			if (score < a)
			{
				position.RevertMove();
				continue;
			}
		}

		int extendedDepth = depth + extension(position, moves[i], alpha, beta);

		int newScore = -NegaScout(position, extendedDepth - SearchIncrement, -b, -a, -colour, distanceFromRoot + 1, true, line);
		if (newScore > a && newScore < beta && i >= 1)
		{	
			newScore = -NegaScout(position, extendedDepth - SearchIncrement, -beta, -alpha, -colour, distanceFromRoot + 1, true, line);	//possibly -newScore rather than -alpha
		}

		position.RevertMove();

		if (newScore > Score)
		{
			Score = newScore;
			bestMove = moves.at(i);
		}

		if (Score > a)
		{
			a = Score;
			UpdatePV(pv, line, moves.at(i));
		}	

		if (a >= beta) //Fail high cutoff
		{
			if (!(moves.at(i) == hashMove))
				AddKiller(moves.at(i), distanceFromRoot);

			break;
		}

		b = a + 1;				//Set a new zero width window
	}

	AddScoreToTable(Score, alpha, position, depth, distanceFromRoot, beta, bestMove);
	return Score;
}

int extension(Position & position, Move & move, int alpha, int beta)
{
	int extension = 0;

	if (IsPV(beta, alpha))
	{
		if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()))	
			extension += SearchIncrement;

		if (position.GetSquare(move.GetTo()) == WHITE_PAWN && GetRank(move.GetTo()) == RANK_7)	//note the move has already been applied
			extension += SearchIncrement;

		if (position.GetSquare(move.GetTo()) == BLACK_PAWN && GetRank(move.GetTo()) == RANK_2)
			extension += SearchIncrement;
	}
	else
	{
		if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()) && move.SEE >= 0)
			extension += SearchIncrement;

		if (position.GetSquare(move.GetTo()) == WHITE_PAWN && GetRank(move.GetTo()) == RANK_7)	//note the move has already been applied
			extension += SearchIncrement;

		if (position.GetSquare(move.GetTo()) == BLACK_PAWN && GetRank(move.GetTo()) == RANK_2)
			extension += SearchIncrement;
	}

	return extension;
}

bool LMR(std::vector<Move>& moves, int i, int beta, int alpha, bool InCheck, Position& position, int depth)
{
	return !moves[i].IsCapture() 
		&& !moves[i].IsPromotion() 
		&& !IsPV(beta, alpha)
		&& !InCheck 
		&& !IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()) 
		&& depth > 3;
}

bool IsFutile(int beta, int alpha, std::vector<Move>& moves, int i, bool InCheck, Position& position)
{
	return !IsPV(beta, alpha)
		&& !moves[i].IsCapture() 
		&& !moves[i].IsPromotion() 
		&& !InCheck 
		&& !IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn());
}

bool AllowedNull(bool allowedNull, Position& position, int beta, int alpha)
{
	return allowedNull
		&& !IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn())
		&& GetBitCount(position.GetAllPieces()) >= 5 //TODO: experiment with this number
		&& !IsPV(beta, alpha);
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
	if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()))
	{
		return (MateScore) + (distanceFromRoot);
	}
	else
	{
		return (Draw);
	}
}

int Quiescence(Position& position, int alpha, int beta, int colour, int distanceFromRoot, std::deque<Move>& pv)
{
	pv.clear();
	if (timeManage.AbortSearch()) return 0;	//checking the time is expensive so we only do it every so often

	int staticScore = colour * EvaluatePosition(position);
	if (staticScore >= beta)
	{
		return staticScore;
	}

	alpha = max(staticScore, alpha);
	std::vector<Move> moves;
	QuiescenceMoves(position, moves);

	if (moves.size() == 0)
		return staticScore;

	int Score = staticScore;
	OrderMoves(moves, position, 0, distanceFromRoot);
	std::deque<Move> line;

	for (int i = 0; i < moves.size(); i++)
	{
		if (moves[i].SEE <= 0 && position.GetCaptureSquare() != moves[i].GetTo())	//prune bad or equal captures that are not recaptures
			continue;

		if (moves[i].SEE < 0 && position.GetCaptureSquare() == moves[i].GetTo())	//prune only bad recaptures and allow equal recaptures. Once SEE < 0 we will skip all remaining
			break;

		if (moves[i].IsPromotion() && !(moves[i].GetFlag() == QUEEN_PROMOTION || moves[i].GetFlag() == QUEEN_PROMOTION_CAPTURE))	//prune underpromotions
			continue;

		if (staticScore + moves[i].SEE + 200 < alpha) break;																		//delta pruning

		position.ApplyMove(moves.at(i));
		Score = max(Score, -Quiescence(position, -beta, -alpha, -colour, distanceFromRoot + 1, line));
		position.RevertMove();

		if (Score > alpha)
		{
			alpha = Score;
			UpdatePV(pv, line, moves.at(i));
		}

		if (Score >= beta)
			return Score;
	}

	return Score;
}

void AddKiller(Move move, int distanceFromRoot)
{
	if (move.IsCapture() || move.IsPromotion()) return;

	if (move == KillerMoves.at(distanceFromRoot)[0]) return;
	if (move == KillerMoves.at(distanceFromRoot)[1])
	{
		//swap to the front
		Move temp = KillerMoves.at(distanceFromRoot)[0];
		KillerMoves.at(distanceFromRoot)[0] = KillerMoves.at(distanceFromRoot)[1];
		KillerMoves.at(distanceFromRoot)[1] = temp;

		return;
	}

	KillerMoves.at(distanceFromRoot)[1] = move;	//replace the 2nd one
}

Move GetHashMove(Position& position, int depth)
{
	if (tTable.CheckEntry(position.GetZobristKey(), depth))
	{
		return tTable.GetEntry(position.GetZobristKey()).GetMove();
	}

	return {};
}