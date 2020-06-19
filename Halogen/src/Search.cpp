#include "Search.h"

const std::vector<int> FutilityMargins = { 100, 150, 250, 400, 600 };
const unsigned int R = 2;	//Null-move reduction depth

TranspositionTable tTable;

void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth, int distanceFromRoot, int alpha, int beta, int colour, ThreadData& locals);
void InternalIterativeDeepening(Move& TTmove, int searchDepth, Position& position, int alpha, int beta, int colour, int distanceFromRoot, ThreadData& locals);
void SortMovesByScore(std::vector<Move>& moves, std::vector<int>& orderScores);
void PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, int score, int alpha, int beta, Position& position, Move move, ThreadData& locals);
void PrintBestMove(Move& Best);
bool UseTransposition(TTEntry& entry, int distanceFromRoot, int alpha, int beta);
bool CheckForRep(Position& position);
bool LMR(std::vector<Move>& moves, int i, int beta, int alpha, bool InCheck, Position& position, int depth);
bool IsFutile(int beta, int alpha, std::vector<Move>& moves, int i, bool InCheck, Position& position);
bool AllowedNull(bool allowedNull, Position& position, int beta, int alpha, int depth);
bool IsEndGame(const Position& position);
bool IsPV(int beta, int alpha);
void AddScoreToTable(int Score, int alphaOriginal, Position& position, int depth, int distanceFromRoot, int beta, Move bestMove);
void UpdateBounds(TTEntry& entry, int& alpha, int& beta);
int TerminalScore(Position& position, int distanceFromRoot);
int extension(Position & position, Move & move, int alpha, int beta);
Move GetHashMove(Position& position, int depth);
Move GetHashMove(Position& position);
void AddKiller(Move move, int distanceFromRoot, std::vector<Killer>& KillerMoves);
void AddHistory(Move& move, int depth, unsigned int (&HistoryMatrix)[N_SQUARES][N_SQUARES]);
void UpdatePV(Move move, int distanceFromRoot, std::vector<std::vector<Move>>& PvTable);

Move SearchPosition(Position position, int allowedTimeMs, uint64_t& totalNodes, int maxSearchDepth = MAX_DEPTH, ThreadData locals = ThreadData());
SearchResult NegaScout(Position& position, int depth, int alpha, int beta, int colour, int distanceFromRoot, bool allowedNull, ThreadData& locals, bool printMoves = false);
SearchResult Quiescence(Position& position, int alpha, int beta, int colour, int distanceFromRoot, int depth, ThreadData& locals);

int see(Position& position, int square, bool side);
int seeCapture(Position& position, const Move& move, bool side); //Don't send this an en passant move!

void InitSearch();


Move BeginSearch(Position position, int allowedTimeMs, int maxSearchDepth)
{
	InitSearch();

	uint64_t nodesSearched = 0;

	std::thread SearchThread([&] {SearchPosition(position, allowedTimeMs, nodesSearched); });
	SearchThread.detach();
}

uint64_t BenchSearch(Position position, int maxSearchDepth)
{
	InitSearch();

	uint64_t nodesSearched = 0;
	SearchPosition(position, 2147483647, nodesSearched, maxSearchDepth);
	return nodesSearched;
}

void InitSearch()
{
	KeepSearching = true;
	tTable.SetAllAncient();
	tTable.ResetHitCount();
	pawnHashTable.HashHits = 0;
	pawnHashTable.HashMisses = 0;
}

void OrderMoves(std::vector<Move>& moves, Position& position, int searchDepth, int distanceFromRoot, int alpha, int beta, int colour, ThreadData& locals)
{
	/*
	We want to order the moves such that the best moves are more likely to be further towards the front.

	The order is as follows:

	1. Hash move												= 10m
	2. Queen Promotions											= 9m
	3. Winning captures											= +8m
	4. Killer moves												= ~7m
	5. Losing captures											= -6m
	6. Quiet moves (further sorted by history matrix values)	= 0-1m
	7. Underpromotions											= -1

	Note that typically the maximum value of the history matrix does not exceed 1,000,000 after a minute
	and as such we choose 1m to be the maximum allowed value

	*/

	Move TTmove = GetHashMove(position);

	//basically, if we have no hash move, do a shallow search and make that the hash move
	InternalIterativeDeepening(TTmove, searchDepth, position, alpha, beta, colour, distanceFromRoot, locals);

	std::vector<int> orderScores(moves.size(), 0);

	for (int i = 0; i < moves.size(); i++)
	{
		//Hash move
		if (moves[i] == TTmove)
		{
			orderScores[i] = 10000000;
			continue;
		}

		//Promotions
		if (moves[i].IsPromotion()) 
		{
			if (moves[i].GetFlag() == QUEEN_PROMOTION || moves[i].GetFlag() == QUEEN_PROMOTION_CAPTURE)
			{
				orderScores[i] = 9000000;
			}
			else
			{
				orderScores[i] = -1;	
			}

			continue;
		}

		//Captures
		if (moves[i].IsCapture())
		{
			int SEE = 0;

			if (moves[i].GetFlag() != EN_PASSANT)
			{
				SEE = seeCapture(position, moves[i], colour);
			}

			if (SEE >= 0)
			{
				orderScores[i] = 8000000 + SEE;
			}

			if (SEE < 0)
			{
				orderScores[i] = 6000000 + SEE;
			}

			continue;
		}

		//Killers
		if (moves[i] == locals.KillerMoves.at(distanceFromRoot).move[0])
		{
			orderScores[i] = 7500000;
			continue;
		}

		if (moves[i] == locals.KillerMoves.at(distanceFromRoot).move[1])
		{
			orderScores[i] = 6500000;
			continue;
		}

		//Quiet
		orderScores[i] = locals.HistoryMatrix[moves[i].GetFrom()][moves[i].GetTo()];

		if (orderScores[i] > 1000000)
		{
			orderScores[i] = 1000000;
		}
	}

	SortMovesByScore(moves, orderScores);
}

void InternalIterativeDeepening(Move& TTmove, int searchDepth, Position& position, int alpha, int beta, int colour, int distanceFromRoot, ThreadData& locals)
{
	if (TTmove.GetFlag() == UNINITIALIZED && searchDepth > 3)
	{
		TTmove = NegaScout(position, searchDepth - 2, alpha, beta, colour, distanceFromRoot, true, locals).GetMove();
	}
}

void SortMovesByScore(std::vector<Move>& moves, std::vector<int>& orderScores)
{
	//selection sort
	for (int i = 0; i < moves.size() - 1; i++)
	{
		int max = i;

		for (int j = i + 1; j < moves.size(); j++)
		{
			if (orderScores[j] > orderScores[max])
			{
				max = j;
			}
		}

		if (max != i)
		{
			std::swap(moves[i], moves[max]);
			std::swap(orderScores[i], orderScores[max]);
		}
	}
}

int see(Position& position, int square, bool side)
{
	int value = 0;
	Move capture = GetSmallestAttackerMove(position, square, side);
	
	if (capture.GetFlag() != UNINITIALIZED)
	{
		int captureValue = PieceValues[position.GetSquare(capture.GetTo())];

		position.ApplySEECapture(capture);
		value = std::max(0, captureValue - see(position, square, !side));	// Do not consider captures if they lose material, therefor max zero 
		position.RevertSEECapture();
	}

	return value;
}

int seeCapture(Position& position, const Move& move, bool side)
{
	assert(move.GetFlag() == CAPTURE);	//Don't seeCapture with promotions or en_passant!

	int value = 0;
	int captureValue = PieceValues[position.GetSquare(move.GetTo())];

	position.ApplySEECapture(move);
	value = captureValue - see(position, move.GetTo(), !side);
	position.RevertSEECapture();

	return value;
}


void PrintBestMove(Move& Best)
{
	std::cout << "bestmove ";
	Best.Print();
	std::cout << std::endl;
}

void PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, int score, int alpha, int beta, Position& position, Move move, ThreadData& locals)
{
	uint64_t actualNodeCount = position.GetNodeCount();
	std::vector<Move> pv = locals.PvTable[0];

	if (pv.size() == 0)
		pv.push_back(move);

	std::cout
		<< "info depth " << depth																//the depth of search
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
		<< " nps " << int(actualNodeCount / std::max(int(Time), 1) * 1000)
		<< " hashfull " << int(float(tTable.GetCapacity()) / tTable.GetSize() * 1000)							//thousondths full
		<< " hashHitRate " << tTable.GetHitCount() * 1000 / std::max(actualNodeCount, uint64_t(1))
		<< " pawnHitRate " << pawnHashTable.HashHits * 1000 / std::max(pawnHashTable.HashHits + pawnHashTable.HashMisses, uint64_t(1))
		<< " pv ";																								//the current best line found



	for (int i = 0; i < pv.size(); i++)
	{
		pv[i].Print();
		std::cout << " ";
	}

	std::cout << std::endl;
}

Move SearchPosition(Position position, int allowedTimeMs, uint64_t& totalNodes, int maxSearchDepth, ThreadData locals)
{
	Move move;

	locals.timeManage.StartSearch(allowedTimeMs);
	position.ResetNodeCount();

	Timer searchTime;
	searchTime.Start();

	int alpha = -30000;
	int beta = 30000;
	int prevScore = 0;

	for (int depth = 1; (!locals.timeManage.AbortSearch(position.GetNodeCount()) && locals.timeManage.ContinueSearch() && depth <= maxSearchDepth) || depth == 1; )	//depth == 1 is a temporary band-aid to illegal moves under time pressure.
	{
		position.IncreaseNodeCount();	//make the root node count. Otherwise when re-searching a position and getting an immediant hash hit the nodes searched is zero

		SearchResult search = NegaScout(position, depth, alpha, beta, position.GetTurn() ? 1 : -1, 0, false, locals, searchTime.ElapsedMs() > 1000);
		int score = search.GetScore();
		Move returnMove = search.GetMove();

		if (locals.timeManage.AbortSearch(position.GetNodeCount())) { break; }

		if (score <= alpha)
		{
			PrintSearchInfo(depth, searchTime.ElapsedMs(), abs(alpha) > 9000, alpha, alpha, beta, position, move, locals);
			alpha = std::max(int(LowINF), prevScore - abs(prevScore - alpha) * 4);
			continue;
		}

		if (score >= beta)
		{
			PrintSearchInfo(depth, searchTime.ElapsedMs(), abs(beta) > 9000, beta, alpha, beta, position, move, locals);
			beta = std::min(int(HighINF), prevScore + abs(prevScore - beta) * 4);
			continue;
		}

		move = returnMove;	//this is only hit if the continue before is not hit
		PrintSearchInfo(depth, searchTime.ElapsedMs(), abs(score) > 9000, score, alpha, beta, position, move, locals);

		depth++;
		alpha = score - 25;
		beta = score + 25;
		prevScore = score;
	}

	//tTable.RunAsserts();	//only for testing purposes

	PrintBestMove(move);
	totalNodes = position.GetNodeCount();
	return move;
}

SearchResult NegaScout(Position& position, int depth, int alpha, int beta, int colour, int distanceFromRoot, bool allowedNull, ThreadData& locals, bool printMoves)
{
#ifdef _DEBUG
	/*Add any code in here that tests the position for validity*/
	position.GetKing(WHITE);	//this has internal asserts
	position.GetKing(BLACK);
	assert((colour == 1 && position.GetTurn() == WHITE) || (colour == -1 && position.GetTurn() == BLACK));
#endif 

	locals.PvTable[distanceFromRoot].clear();

	if (locals.timeManage.AbortSearch(position.GetNodeCount())) return -1;		//we must check later that we don't let this score pollute the transposition table
	if (distanceFromRoot >= MAX_DEPTH) return 0;						//If we are 100 moves from root I think we can assume its a drawn position

	//check for draw
	if (DeadPosition(position)) return 0;
	if (CheckForRep(position)) return 0;

	/*Query the transpotition table*/
	if (tTable.CheckEntry(position.GetZobristKey(), depth))
	{
		int rep = 1;
		uint64_t current = position.GetZobristKey();
		
		for (int i = 0; i < position.GetPreviousKeysSize(); i++)	//note Previous keys will not contain the current key, hence rep starts at one
		{
			if (position.GetPreviousKey(i) == current)
			{
				rep++;
				break;
			}
		}

		if (rep < 2)												//don't use the transposition if we have been at this position in the past
		{
			TTEntry entry = tTable.GetEntry(position.GetZobristKey());
			if (UseTransposition(entry, distanceFromRoot, alpha, beta)) return SearchResult(entry.GetScore(), entry.GetMove());
		}
	}

	/*Drop into quiescence search*/
	if (depth <= 0 && !IsInCheck(position))
	{ 
		return Quiescence(position, alpha, beta, colour, distanceFromRoot, depth, locals);
	}

	/*Null move pruning*/
	if (AllowedNull(allowedNull, position, beta, alpha, depth))
	{
		position.ApplyNullMove();
		int score = -NegaScout(position, depth - R - 1, -beta, -beta + 1, -colour, distanceFromRoot + 1, false, locals).GetScore();	
		position.RevertNullMove();

		//Verification search worth about ~5 elo. 
		if (score >= beta)
		{
			SearchResult result = NegaScout(position, depth - R - 1, beta - 1, beta, colour, distanceFromRoot, false, locals);

			if (result.GetScore() >= beta)
				return result;
		}
	}

	std::vector<Move> moves;
	LegalMoves(position, moves);

	if (moves.size() == 0)
	{
		return TerminalScore(position, distanceFromRoot);
	}

	if (position.GetFiftyMoveCount() >= 100) return 0;	//must make sure its not already checkmate

	//mate distance pruning
	alpha = std::max<int>(Score::MateScore + distanceFromRoot, alpha);
	beta = std::min<int>(-Score::MateScore - distanceFromRoot - 1, beta);
	if (alpha >= beta)
		return alpha;

	OrderMoves(moves, position, depth, distanceFromRoot, alpha, beta, colour, locals);
	Move bestMove = Move();	//used for adding to transposition table later
	int Score = LowINF;
	int a = alpha;
	int b = beta;
	bool InCheck = IsInCheck(position);
	int staticScore = colour * EvaluatePosition(position);

	for (int i = 0; i < moves.size(); i++)	
	{
		if (printMoves) {
			std::cout << "info currmovenumber " << i + 1 << " currmove ";
			moves[i].Print();
			std::cout << "\n";
		}

		position.ApplyMove(moves.at(i));
		tTable.PreFetch(position.GetZobristKey());							//load the transposition into l1 cache. ~5% speedup

		//futility pruning
		if (IsFutile(beta, alpha, moves, i, InCheck, position) && i > 0)	//Possibly stop futility pruning if alpha or beta are close to mate scores
		{
			if (depth < FutilityMargins.size() && staticScore + FutilityMargins.at(std::max<int>(0, depth)) < a)
			{
				position.RevertMove();
				continue;
			}
		}

		int extendedDepth = depth + extension(position, moves[i], alpha, beta);

		//late move reductions
		if (extendedDepth == depth && LMR(moves, i, beta, alpha, InCheck, position, depth) && i > 3)
		{
			int score = -NegaScout(position, extendedDepth - 2, -a - 1, -a, -colour, distanceFromRoot + 1, true, locals).GetScore();

			if (score < a)
			{
				position.RevertMove();
				continue;
			}
		}

		int newScore = -NegaScout(position, extendedDepth - 1, -b, -a, -colour, distanceFromRoot + 1, true, locals).GetScore();
		if (newScore > a && newScore < beta && i >= 1)
		{	
			newScore = -NegaScout(position, extendedDepth - 1, -beta, -a, -colour, distanceFromRoot + 1, true, locals).GetScore();
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
			UpdatePV(moves.at(i), distanceFromRoot, locals.PvTable);
		}

		if (a >= beta) //Fail high cutoff
		{
			AddKiller(moves.at(i), distanceFromRoot, locals.KillerMoves);
			AddHistory(moves[i], depth, locals.HistoryMatrix);
			break;
		}

		b = a + 1;				//Set a new zero width window
	}

	if (!locals.timeManage.AbortSearch(position.GetNodeCount()))
		AddScoreToTable(Score, alpha, position, depth, distanceFromRoot, beta, bestMove);

	return SearchResult(Score, bestMove);
}

void UpdatePV(Move move, int distanceFromRoot, std::vector<std::vector<Move>>& PvTable)
{
	PvTable[distanceFromRoot].clear();
	PvTable[distanceFromRoot].push_back(move);

	if (distanceFromRoot + 1 < PvTable.size())
		PvTable[distanceFromRoot].insert(PvTable[distanceFromRoot].end(), PvTable[distanceFromRoot + 1].begin(), PvTable[distanceFromRoot + 1].end());
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
	int totalRep = 1;
	uint64_t current = position.GetZobristKey();

	//note Previous keys will not contain the current key, hence rep starts at one
	for (int i = 0; i < position.GetPreviousKeysSize(); i++)
	{
		if (position.GetPreviousKey(i) == current)
		{
			totalRep++;
		}

		if (totalRep == 3) return true;	
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
		int SEE = see(position, move.GetTo(), position.GetTurn());

		if (IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn()) && SEE == 0)	//move already applied so positive SEE bad
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
		//&& !IsPV(beta, alpha)
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
		&& !IsEndGame(position)
		&& depth > R + 1								//don't drop directly into quiessence search. particularly important in mate searches as quiessence search has no mate detection currently. See 5rk1/2p4p/2p4r/3P4/4p1b1/1Q2NqPp/PP3P1K/R4R2 b - - 0 1
		&& GetBitCount(position.GetAllPieces()) >= 5;	//avoid null move pruning in very late game positions due to zanauag issues. Even with verification search e.g 8/6k1/8/8/8/8/1K6/Q7 w - - 0 1 
}

bool IsEndGame(const Position& position)
{
	return (position.GetAllPieces() == (position.GetPieceBB(WHITE_KING) | position.GetPieceBB(BLACK_KING) | position.GetPieceBB(WHITE_PAWN) | position.GetPieceBB(BLACK_PAWN)));
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
		alpha = std::max<int>(alpha, entry.GetScore());
	}

	if (entry.GetCutoff() == EntryType::UPPERBOUND)
	{
		beta = std::min<int>(beta, entry.GetScore());
	}
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

SearchResult Quiescence(Position& position, int alpha, int beta, int colour, int distanceFromRoot, int depth, ThreadData& locals)
{
	locals.PvTable[distanceFromRoot].clear();

	if (locals.timeManage.AbortSearch(position.GetNodeCount())) return -1;
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
		
	OrderMoves(moves, position, depth, distanceFromRoot, alpha, beta, colour, locals);

	for (int i = 0; i < moves.size(); i++)
	{
		int SEE = 0;
		if (moves[i].GetFlag() == CAPTURE) //seeCapture doesn't work for ep or promotions
		{
			SEE = seeCapture(position, moves[i], colour);
		}

		if (moves[i].IsPromotion())
		{
			SEE += PieceValues[WHITE_QUEEN];
		}

		if (staticScore + SEE + 200 < alpha) 								//delta pruning
			break;

		if (SEE < 0)														//prune bad captures
			break;

		if (SEE <= 0 && position.GetCaptureSquare() != moves[i].GetTo())	//prune equal captures that aren't recaptures
			continue;

		if (moves[i].IsPromotion() && !(moves[i].GetFlag() == QUEEN_PROMOTION || moves[i].GetFlag() == QUEEN_PROMOTION_CAPTURE))	//prune underpromotions
			continue;

		position.ApplyMove(moves.at(i));
		int newScore = -Quiescence(position, -beta, -alpha, -colour, distanceFromRoot + 1, depth - 1, locals).GetScore();
		position.RevertMove();

		if (newScore > Score)
		{
			bestmove = moves.at(i);
			Score = newScore;
		}

		if (Score > alpha)
		{
			alpha = Score;
			UpdatePV(moves.at(i), distanceFromRoot, locals.PvTable);
		}

		if (Score >= beta)
			break;
	}

	return SearchResult(Score, bestmove);
}

void AddKiller(Move move, int distanceFromRoot, std::vector<Killer>& KillerMoves)
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

void AddHistory(Move& move, int depth, unsigned int(&HistoryMatrix)[N_SQUARES][N_SQUARES])
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

ThreadData::ThreadData() : HistoryMatrix{{0}}
{
	PvTable.clear();
	for (int i = 0; i < MAX_DEPTH; i++)
	{
		PvTable.push_back(std::vector<Move>());
	}

	for (int i = 0; i < MAX_DEPTH; i++)
	{
		KillerMoves.push_back(Killer());
	}
}

