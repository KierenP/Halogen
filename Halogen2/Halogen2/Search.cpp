#include "Search.h"

enum class SearchLevels
{
	ALPHABETA,
	TERMINATE,
	QUIETESSENCE,			//captures and all moves if in check
	LEAF_SEARCH,			//only look at captures of hanged pieces
	CHECK_EXTENSION
};

TranspositionTable tTable;
SearchTimeManage timeManage;


void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth);
bool CheckForDraw(ABnode*& node, Position& position);
void SwapMoves(std::vector<Move>& moves, unsigned int a, unsigned int b);
void PrintSearchInfo(Position& position, unsigned int depth, double Time, bool isCheckmate, std::deque<Move> pv, int score);
int int_pow(int base, int exp); //Note pow() as part of the std uses the double data type and hence is not suitable for integer powers due to the possibility of 2 ^ 5 = 31.9999999999997 = 31 for example
void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth);
void PrintBestMove(Move& Best);
int NegaScout(Position& position, int depth, int alpha, int beta, int colour, int distanceFromRoot, std::deque<Move>& pv);
void AddScoreToTable(int Score, int alphaOriginal, Position& position, int depth, int distanceFromRoot, int beta, Move bestMove);
void UpdateBounds(TTEntry& entry, int& alpha, int& beta);
void UpdatePV(std::deque<Move>& pv, std::deque<Move>& line, std::vector<Move>& moves, int i);
int TerminalScore(Position& position, int distanceFromRoot);
int Quiescence(Position& position, int alpha, int beta, int colour, int distanceFromRoot, std::deque<Move>& pv);


//stolen from stack-overflow. Uses the 'exponentiation by squaring' technique which is faster
int int_pow(int base, int exp)
{
	if (exp == 0) return 1;
	if (exp == 1) return base;

	int tmp = int_pow(base, exp / 2);
	if (exp % 2 == 0) return tmp * tmp;
	else return base * tmp * tmp;
}

void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth)
{
	/*
	We want to order the moves such that the best moves are more likely to be further towards the front.
	*/

	Move TTmove;
	std::vector<int> RelativePieceValues = { 100, 300, 300, 500, 900, 10000, 100, 300, 300, 500, 900, 10000 };	//technically, there is no way we can be capturing a king as the game would already be over.

	if (tTable.CheckEntry(position.GetZobristKey(), searchDepth - 1))
	{
		TTmove = tTable.GetEntry(position.GetZobristKey()).GetMove();
	}

	//give each move a score on how 'good' it is. 
	for (int i = 0; i < moves.size(); i++)
	{
		if (moves[i] == TTmove) {
			moves[i].SEE = 15000;	//we want this at the front
			continue;
		}

		if (!moves[i].IsCapture() && !moves[i].IsPromotion()) {
			continue;	//quite moves get a score of zero, but may come before some bad captures which get a negative score
		}

		if (moves[i].IsCapture())
		{
			int PieceValue = RelativePieceValues.at(position.GetSquare(moves[i].GetFrom()));
			int CaptureValue = 0;

			if (moves[i].GetFlag() == EN_PASSANT)
				CaptureValue = RelativePieceValues[WHITE_PAWN];	//1

			if (moves[i].IsCapture() && moves[i].GetFlag() != EN_PASSANT)
			{
				CaptureValue = RelativePieceValues.at(position.GetSquare(moves[i].GetTo()));	//if enpassant then the .GetTo() square is empty
			}

			moves[i].SEE += CaptureValue;

			if (IsInCheck(position, moves[i].GetTo(), position.GetTurn()))
				moves[i].SEE -= PieceValue;
		}

		if (moves[i].IsPromotion())
		{
			moves[i].SEE = 0;
			continue;	//todo: fix how promotions work and remove this line and the one above

			if (moves[i].GetFlag() == KNIGHT_PROMOTION || moves[i].GetFlag() == KNIGHT_PROMOTION_CAPTURE)
				moves[i].SEE += RelativePieceValues[WHITE_KNIGHT];
			if (moves[i].GetFlag() == ROOK_PROMOTION || moves[i].GetFlag() == ROOK_PROMOTION_CAPTURE)
				moves[i].SEE += RelativePieceValues[WHITE_ROOK];
			if (moves[i].GetFlag() == BISHOP_PROMOTION || moves[i].GetFlag() == BISHOP_PROMOTION_CAPTURE)
				moves[i].SEE += RelativePieceValues[WHITE_BISHOP];
			if (moves[i].GetFlag() == QUEEN_PROMOTION || moves[i].GetFlag() == QUEEN_PROMOTION_CAPTURE)
				moves[i].SEE += RelativePieceValues[WHITE_QUEEN];

			if (!moves[i].IsCapture())
				moves[i].SEE -= RelativePieceValues[WHITE_PAWN];
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

void PrintSearchInfo(Position& position, unsigned int depth, double Time, bool isCheckmate, std::deque<Move> pv, int score)
{
	std::cout
		<< "info depth " << depth																				//the depth of search
		<< " seldepth " << depth;															//the selective depth (for example searching further for checks and captures)

	if (isCheckmate)
	{
		std::cout << " score mate " << (min(abs(static_cast<int>(Score::BlackWin) - score), abs(static_cast<int>(Score::WhiteWin) - score))) / 2;
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

void SwapMoves(std::vector<Move>& moves, unsigned int a, unsigned int b)
{
	assert(a < moves.size());
	assert(b < moves.size());

	Move temp = moves[b];
	moves[b] = moves[a];
	moves[a] = temp;
}

bool CheckForDraw(ABnode*& node, Position& position)
{
	if (InsufficentMaterial(position)) return 0;
	//TODO: potential early break from loop and speedup!

	int rep = 1;
	uint64_t current = position.GetZobristKey();

	//note Previous keys will not contain the current key
	for (int i = 0; i < PreviousKeys.size(); i++)
	{
		if (PreviousKeys.at(i) == current)
			rep++;
	}

	//we want to put a 2 fold rep as a draw to avoid the program delaying the inevitable and thinking repeating a position might be good/bad
	if (rep >= 2)
	{
		node->SetScore(0);
		node->SetCutoff(NodeCut::THREE_FOLD_REP_CUTOFF);
		return true;
	}
	return false;
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

	for (int depth = 1; !timeManage.AbortSearch() && timeManage.ContinueSearch(); )
	{
		std::deque<Move> pv;
		int score = NegaScout(position, depth, alpha, beta, position.GetTurn() ? 1 : -1, 1, pv);

		if (timeManage.AbortSearch()) {	break; }

		if (score <= alpha || score >= beta)
		{
			alpha = -30000;
			beta = 30000;
			std::cout << "info re-search\n";
			continue;
		}

		depth++;			//this is only hit if the continue before is not hit
		move = pv[0];
		PrintSearchInfo(position, depth, searchTime.ElapsedMs(), abs(score) > 9000, pv, score);

		alpha = score - 25;
		beta = score + 25;
	}

	PrintBestMove(move);
	return move;
}

int NegaScout(Position& position, int depth, int alpha, int beta, int colour, int distanceFromRoot, std::deque<Move>& pv)
{
	pv.clear();

	if (timeManage.AbortSearch()) return 0;
	int alphaOriginal = alpha;

	/*Query the transpotition table*/
	if (tTable.CheckEntry(position.GetZobristKey(), depth))
	{
		tTable.AddHit();

		TTEntry entry = tTable.GetEntry(position.GetZobristKey());
		entry.MateScoreAdjustment(distanceFromRoot);	//this MUST be done
		pv.push_front(entry.GetMove());

		if (entry.GetCutoff() == EntryType::EXACT) return entry.GetScore();

		UpdateBounds(entry, alpha, beta);

		if (alpha >= beta)
			return entry.GetScore();
	}

	std::deque<Move> line;
	if (depth == 0) 
	{ 
		return Quiescence(position, alpha, beta, colour, distanceFromRoot, line);
	}

	std::vector<Move> moves = GenerateLegalMoves(position);

	if (moves.size() == 0)
	{
		return TerminalScore(position, distanceFromRoot);
	}

	OrderMoves(moves, position, depth);
	Move bestMove = Move();	//used for adding to transposition table later
	int Score = -30000;
	int a = alpha;	//possibly a = alphaOriginal? 
	int b = beta;

	for (int i = 0; i < moves.size(); i++)
	{
		position.ApplyMove(moves.at(i));

		int newScore = -NegaScout(position, depth - 1, -b, -a, -colour, distanceFromRoot + 1, line);
		if (newScore > a && newScore < beta && i > 1)
		{	
			newScore = -NegaScout(position, depth - 1, -beta, -alpha, -colour, distanceFromRoot + 1, line);	//possibly -newScore rather than -alpha
		}

		position.RevertMove(moves.at(i));

		Score = max(Score, newScore);

		if (Score > a)
		{
			a = Score;
			UpdatePV(pv, line, moves, i);
			bestMove = moves.at(i);
		}	

		if (a >= beta) break;	//Fail high cutoff
		b = a + 1;				//Set a new zero width window
	}

	AddScoreToTable(Score, alphaOriginal, position, depth, distanceFromRoot, beta, bestMove);
	return Score;
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

void UpdatePV(std::deque<Move>& pv, std::deque<Move>& line, std::vector<Move>& moves, int i)
{
	pv = line;
	pv.push_front(moves.at(i));
}

int TerminalScore(Position& position, int distanceFromRoot)
{
	if (IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn()))
	{
		return static_cast<int>(Score::BlackWin) + distanceFromRoot;
	}
	else
	{
		return static_cast<int>(Score::Draw);
	}
}

int Quiescence(Position& position, int alpha, int beta, int colour, int distanceFromRoot, std::deque<Move>& pv)
{
	std::deque<Move> line;

	int staticScore = colour * EvaluatePosition(position);

	if (staticScore >= beta)
	{
		pv.clear();
		return staticScore;
	}

	alpha = max(staticScore, alpha);
	std::vector<Move> moves = GenerateLegalCaptures(position);

	int Score = staticScore;
	OrderMoves(moves, position, 0);

	for (int i = 0; i < moves.size(); i++)
	{
		if (moves[i].SEE < 0)
			break;

		if (staticScore + moves[i].SEE + 200 < alpha) break;	//delta pruning

		position.ApplyMove(moves.at(i));
		Score = max(Score, -Quiescence(position, -beta, -alpha, -colour, distanceFromRoot + 1, line));
		position.RevertMove(moves.at(i));

		if (Score >= beta)
			return Score;

		if (Score > alpha)
		{
			alpha = Score;
			UpdatePV(pv, line, moves, i);
		}
		line.clear();
	}

	return Score;
}