#include "Search.h"

int FutilityMargins[MAX_DEPTH];
const unsigned int R = 3;					//Null-move reduction depth
const unsigned int VariableNullDepth = 7;	//Beyond this depth R = 4

TranspositionTable tTable;

void OrderMoves(std::vector<Move>& moves, Position& position, int distanceFromRoot, SearchData& locals);
void PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, int score, int alpha, int beta, const Position& position, const Move& move, const SearchData& locals, const ThreadSharedData& sharedData);
void PrintBestMove(Move Best);
bool UseTransposition(TTEntry& entry, int distanceFromRoot, int alpha, int beta);
bool CheckForRep(Position& position, int distanceFromRoot);
bool LMR(bool InCheck, const Position& position);
bool IsFutile(Move move, int beta, int alpha, bool InCheck, const Position& position);
bool AllowedNull(bool allowedNull, const Position& position, int beta, int alpha);
bool IsEndGame(const Position& position);
bool IsPV(int beta, int alpha);
void AddScoreToTable(int Score, int alphaOriginal, const Position& position, int depthRemaining, int distanceFromRoot, int beta, Move bestMove);
void UpdateBounds(const TTEntry& entry, int& alpha, int& beta);
int TerminalScore(const Position& position, int distanceFromRoot);
int extension(Position & position, int alpha, int beta);
Move GetHashMove(const Position& position, int depthRemaining, int distanceFromRoot);
Move GetHashMove(const Position& position, int distanceFromRoot);
void AddKiller(Move move, int distanceFromRoot, std::vector<Killer>& KillerMoves);
void AddHistory(const Move& move, int depthRemaining, unsigned int (&HistoryMatrix)[N_PLAYERS][N_SQUARES][N_SQUARES], bool sideToMove);
void UpdatePV(Move move, int distanceFromRoot, std::vector<std::vector<Move>>& PvTable);
int Reduction(int depth, int i, int alpha, int beta);
int matedIn(int distanceFromRoot);
int mateIn(int distanceFromRoot);
unsigned int ProbeTBRoot(const Position& position);
unsigned int ProbeTBSearch(const Position& position);
SearchResult UseSearchTBScore(unsigned int result, int staticEval);
SearchResult UseRootTBScore(unsigned int result, int staticEval);

void SearchPosition(Position position, ThreadSharedData& sharedData, unsigned int threadID, int maxTime, int allocatedTimeMs, int maxSearchDepth = MAX_DEPTH, SearchData locals = SearchData());
SearchResult AspirationWindowSearch(Position& position, int depth, int prevScore, SearchData& locals, ThreadSharedData& sharedData, unsigned int threadID, Timer& searchTime);
SearchResult NegaScout(Position& position, unsigned int initialDepth, int depthRemaining, int alpha, int beta, int colour, unsigned int distanceFromRoot, bool allowedNull, SearchData& locals, ThreadSharedData& sharedData);
void UpdateAlpha(int Score, int& a, std::vector<Move>& moves, const size_t& i, unsigned int distanceFromRoot, SearchData& locals);
void UpdateScore(int newScore, int& Score, Move& bestMove, std::vector<Move>& moves, const size_t& i);
SearchResult Quiescence(Position& position, unsigned int initialDepth, int alpha, int beta, int colour, unsigned int distanceFromRoot, int depthRemaining, SearchData& locals, ThreadSharedData& sharedData);

int see(Position& position, int square, bool side);
int seeCapture(Position& position, const Move& move); //Don't send this an en passant move!

void InitSearch();

Move MultithreadedSearch(const Position& position, unsigned int maxTimeMs, unsigned int AllocatedTimeMs, unsigned int threadCount, int maxSearchDepth)
{
	InitSearch();

	std::vector<std::thread> threads;
	ThreadSharedData sharedData(threadCount);

	for (unsigned int i = 0; i < threadCount; i++)
	{
		threads.emplace_back(std::thread([=, &sharedData] {SearchPosition(position, sharedData, i, maxTimeMs, AllocatedTimeMs, maxSearchDepth ); })); 
	}

	for (size_t i = 0; i < threads.size(); i++)
	{
		threads[i].join();
	}

	PrintBestMove(sharedData.GetBestMove());
	return sharedData.GetBestMove();
}

uint64_t BenchSearch(const Position& position, int maxSearchDepth)
{
	InitSearch();
	tTable.ResetTable();
	ThreadSharedData sharedData(1, true);
	
	SearchPosition(position, sharedData, 0, 2147483647, 2147483647, maxSearchDepth);

	return sharedData.getNodes();
}

void DepthSearch(const Position& position, int maxSearchDepth)
{
	InitSearch();
	tTable.ResetTable();
	ThreadSharedData sharedData(1);
	SearchPosition(position, sharedData, 0, 2147483647, 2147483647, maxSearchDepth);
	PrintBestMove(sharedData.GetBestMove());
}

void InitSearch()
{
	KeepSearching = true;

	int Futility_linear = 25;
	int Futility_constant = 100;

	for (int i = 0; i < MAX_DEPTH; i++)
	{
		FutilityMargins[i] = Futility_linear * i + Futility_constant;
	}
}

void OrderMoves(std::vector<Move>& moves, Position& position, int distanceFromRoot, SearchData& locals)
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

	Move TTmove = GetHashMove(position, distanceFromRoot);

	for (size_t i = 0; i < moves.size(); i++)
	{
		//Hash move
		if (moves[i] == TTmove)
		{
			moves[i].orderScore = 10000000;
		}

		//Promotions
		else if (moves[i].IsPromotion()) 
		{
			if (moves[i].GetFlag() == QUEEN_PROMOTION || moves[i].GetFlag() == QUEEN_PROMOTION_CAPTURE)
			{
				moves[i].orderScore = 9000000;
			}
			else
			{
				moves[i].orderScore = -1;
			}
		}

		//Captures
		else if (moves[i].IsCapture())
		{
			int SEE = 0;

			if (moves[i].GetFlag() != EN_PASSANT)
			{
				SEE = seeCapture(position, moves[i]);
			}

			if (SEE >= 0)
			{
				moves[i].orderScore = 8000000 + SEE;
			}

			if (SEE < 0)
			{
				moves[i].orderScore = 6000000 + SEE;
			}
		}

		//Killers
		else if (moves[i] == locals.KillerMoves.at(distanceFromRoot).move[0])
		{
			moves[i].orderScore = 7500000;
		}

		else if (moves[i] == locals.KillerMoves.at(distanceFromRoot).move[1])
		{
			moves[i].orderScore = 6500000;
		}

		//Quiet
		else
		{
			moves[i].orderScore = std::min(1000000U, locals.HistoryMatrix[position.GetTurn()][moves[i].GetFrom()][moves[i].GetTo()]);
		}
	}

	std::stable_sort(moves.begin(), moves.end(), [](const Move& a, const Move& b)
		{
			return a.orderScore > b.orderScore;
		});
}

int see(Position& position, int square, bool side)
{
	int value = 0;
	Move capture = GetSmallestAttackerMove(position, square, side);
	
	if (!capture.IsUninitialized())
	{
		int captureValue = PieceValues(position.GetSquare(capture.GetTo()));

		position.ApplySEECapture(capture);
		value = std::max(0, captureValue - see(position, square, !side));	// Do not consider captures if they lose material, therefor max zero 
		position.RevertSEECapture();
	}

	return value;
}

int seeCapture(Position& position, const Move& move)
{
	assert(move.GetFlag() == CAPTURE);	//Don't seeCapture with promotions or en_passant!

	bool side = position.GetTurn();

	int value = 0;
	int captureValue = PieceValues(position.GetSquare(move.GetTo()));

	position.ApplySEECapture(move);
	value = captureValue - see(position, move.GetTo(), !side);
	position.RevertSEECapture();

	return value;
}


void PrintBestMove(Move Best)
{
	std::cout << "bestmove ";
	Best.Print();
	std::cout << std::endl;
}

void PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, int score, int alpha, int beta, const Position& position, const Move& move, const SearchData& locals, const ThreadSharedData& sharedData)
{
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
		<< " nodes " << sharedData.getNodes()
		<< " nps " << int(sharedData.getNodes() / std::max(int(Time), 1) * 1000)
		<< " hashfull " << tTable.GetCapacity(position.GetTurnCount())						//thousondths full
		<< " tbhits " << sharedData.getTBHits();

#if defined(_MSC_VER) && !defined(NDEBUG)  
	std::cout	//these lines are for debug and not part of official uci protocol
		<< " string thread " << std::this_thread::get_id()
		<< " evalHitRate " << locals.evalTable.hits * 1000 / std::max(locals.evalTable.hits + locals.evalTable.misses, uint64_t(1));
#endif

	std::cout << " pv ";																								//the current best line found

	for (size_t i = 0; i < pv.size(); i++)
	{
		pv[i].Print();
		std::cout << " ";
	}

	std::cout << std::endl;
}

void SearchPosition(Position position, ThreadSharedData& sharedData, unsigned int threadID, int maxTime, int allocatedTimeMs, int maxSearchDepth, SearchData locals)
{
	Timer searchTime;
	searchTime.Start();

	locals.timeManage.StartSearch(maxTime, allocatedTimeMs);

	int alpha = -30000;
	int beta = 30000;
	int prevScore = 0;

	for (int depth = 1; (!locals.AbortSearch(0) && depth <= maxSearchDepth) || depth == 1; depth++)	//depth == 1 is a temporary band-aid to illegal moves under time pressure.
	{
		if (!locals.ContinueSearch())
			sharedData.ReportWantsToStop(threadID);

		sharedData.ReportDepth(depth, threadID);

		SearchResult search = AspirationWindowSearch(position, depth, prevScore, locals, sharedData, threadID, searchTime);
		int score = search.GetScore();

		if (depth > 1 && locals.AbortSearch(0)) { break; }
		if (sharedData.ThreadAbort(depth)) { score = sharedData.GetAspirationScore(); }

		sharedData.ReportResult(depth, searchTime.ElapsedMs(), score, alpha, beta, position, search.GetMove(), locals);
		prevScore = score;
	}
}

SearchResult AspirationWindowSearch(Position& position, int depth, int prevScore, SearchData& locals, ThreadSharedData& sharedData, unsigned int threadID, Timer& searchTime)
{
	int alpha = prevScore - std::max(1, 15 + ((threadID % 2 == 0) ? 1 : -1) * int(4.0 * log2(threadID + 1)));
	int beta = prevScore + std::max(1, 15 + ((threadID % 2 == 0) ? 1 : -1) * int(4.0 * log2(threadID + 1)));
	SearchResult search = { 0 };

	while (!locals.AbortSearch(0) || depth == 1)
	{
		search = NegaScout(position, depth, depth, alpha, beta, position.GetTurn() ? 1 : -1, 0, false, locals, sharedData);
		if (alpha < search.GetScore() && search.GetScore() < beta) break;
		if (sharedData.ThreadAbort(depth)) break;

		if (search.GetScore() <= alpha)
		{
			sharedData.ReportResult(depth, searchTime.ElapsedMs(), alpha, alpha, beta, position, search.GetMove(), locals);
			alpha = std::max(int(LowINF), prevScore - abs(prevScore - alpha) * 4);
		}

		if (search.GetScore() >= beta)
		{
			sharedData.ReportResult(depth, searchTime.ElapsedMs(), beta, alpha, beta, position, search.GetMove(), locals);
			beta = std::min(int(HighINF), prevScore + abs(prevScore - beta) * 4);
		}
	} 

	return search;
}

SearchResult NegaScout(Position& position, unsigned int initialDepth, int depthRemaining, int alpha, int beta, int colour, unsigned int distanceFromRoot, bool allowedNull, SearchData& locals, ThreadSharedData& sharedData)
{
#ifdef _DEBUG
	/*Add any code in here that tests the position for validity*/
	position.GetKing(WHITE);	//this has internal asserts
	position.GetKing(BLACK);
	assert((colour == 1 && position.GetTurn() == WHITE) || (colour == -1 && position.GetTurn() == BLACK));
#endif 

	locals.PvTable[distanceFromRoot].clear();

	if (initialDepth > 1 && locals.AbortSearch(position.GetNodes())) return -1;										//we must check later that we don't let this score pollute the transposition table
	if (sharedData.ThreadAbort(initialDepth)) return -1;												//another thread has finished searching this depth: ABORT!
	if (distanceFromRoot >= MAX_DEPTH) return 0;														//If we are 100 moves from root I think we can assume its a drawn position

	//check for draw
	if (DeadPosition(position)) return 0;
	if (CheckForRep(position, distanceFromRoot)) return 0;

	if (distanceFromRoot == 0 && GetBitCount(position.GetAllPieces()) <= TB_LARGEST)
	{
		//at root
		unsigned int result = ProbeTBRoot(position);
		if (result != TB_RESULT_FAILED)
		{
			position.addTbHit();
			if (position.TbHitaddToThreadTotal()) sharedData.AddTBHitChunk();
			return UseRootTBScore(result, colour * EvaluatePositionNet(position, locals.evalTable));
		}
	}

	if (distanceFromRoot > 0 && GetBitCount(position.GetAllPieces()) <= TB_LARGEST)
	{
		//not root
		unsigned int result = ProbeTBSearch(position);
		if (result != TB_RESULT_FAILED)
		{
			position.addTbHit();
			if (position.TbHitaddToThreadTotal()) sharedData.AddTBHitChunk();
			return UseSearchTBScore(result, colour * EvaluatePositionNet(position, locals.evalTable));
		}
	}

	/*Query the transpotition table*/
	if (!IsPV(beta, alpha)) 
	{
		TTEntry entry = tTable.GetEntry(position.GetZobristKey());
		if (CheckEntry(entry, position.GetZobristKey(), depthRemaining))
		{
			tTable.SetNonAncient(position.GetZobristKey(), position.GetTurnCount(), distanceFromRoot);

			int rep = 1;
			uint64_t current = position.GetZobristKey();

			for (unsigned int i = 0; i < position.GetPreviousKeysSize(); i++)	//note Previous keys will not contain the current key, hence rep starts at one
			{
				if (position.GetPreviousKey(i) == current)
				{
					rep++;
					break;
				}
			}

			if (rep < 2)												//don't use the transposition if we have been at this position in the past
			{
				if (UseTransposition(entry, distanceFromRoot, alpha, beta)) return SearchResult(entry.GetScore(), entry.GetMove());
			}
		}
	}

	/*Drop into quiescence search*/
	if (depthRemaining <= 0 && !IsInCheck(position))
	{ 
		return Quiescence(position, initialDepth, alpha, beta, colour, distanceFromRoot, depthRemaining, locals, sharedData);
	}

	int staticScore = colour * EvaluatePositionNet(position, locals.evalTable);

	/*Null move pruning*/
	if (AllowedNull(allowedNull, position, beta, alpha) && (staticScore > beta))
	{
		unsigned int reduction = R + (depthRemaining >= static_cast<int>(VariableNullDepth));

		position.ApplyNullMove();
		int score = -NegaScout(position, initialDepth, depthRemaining - reduction - 1, -beta, -beta + 1, -colour, distanceFromRoot + 1, false, locals, sharedData).GetScore();
		position.RevertNullMove();

		if (score >= beta)
			return beta;
	}

	//mate distance pruning
	alpha = std::max<int>(matedIn(distanceFromRoot), alpha);
	beta = std::min<int>(mateIn(distanceFromRoot), beta);
	if (alpha >= beta)
		return alpha;

	Move bestMove = Move();	//used for adding to transposition table later
	int Score = LowINF;
	int a = alpha;
	int b = beta;

	/*If a hash move exists, search with that move first and hope we can get a cutoff*/
	Move hashMove = GetHashMove(position, distanceFromRoot);
	if (!hashMove.IsUninitialized() && position.GetFiftyMoveCount() < 100 && MoveIsLegal(position, hashMove))	//if its 50 move rule we need to skip this and figure out if its checkmate or draw below
	{
		position.ApplyMove(hashMove);
		tTable.PreFetch(position.GetZobristKey());							//load the transposition into l1 cache. ~5% speedup
		int extendedDepth = depthRemaining + extension(position, alpha, beta);
		int newScore = -NegaScout(position, initialDepth, extendedDepth - 1, -b, -a, -colour, distanceFromRoot + 1, true, locals, sharedData).GetScore();
		position.RevertMove();

		if (position.NodesSearchedAddToThreadTotal()) sharedData.AddNodeChunk();

		if (newScore > Score)
		{
			Score = newScore;
			bestMove = hashMove;
		}

		if (Score > a)
		{
			a = Score;
			UpdatePV(hashMove, distanceFromRoot, locals.PvTable);
		}

		if (a >= beta) //Fail high cutoff
		{
			AddKiller(hashMove, distanceFromRoot, locals.KillerMoves);
			AddHistory(hashMove, depthRemaining, locals.HistoryMatrix, position.GetTurn());

			if (!locals.AbortSearch(position.GetNodes()) && !(sharedData.ThreadAbort(initialDepth)))
				AddScoreToTable(Score, alpha, position, depthRemaining, distanceFromRoot, beta, bestMove);

			return SearchResult(Score, bestMove);
		}

		b = a + 1;				//Set a new zero width window
	}

	std::vector<Move> moves;
	LegalMoves(position, moves);

	if (moves.size() == 0)
	{
		return TerminalScore(position, distanceFromRoot);
	}

	if (position.GetFiftyMoveCount() >= 100) return 0;	//must make sure its not already checkmate
	
	OrderMoves(moves, position, distanceFromRoot, locals);
	bool InCheck = IsInCheck(position);

	if (hashMove.IsUninitialized() && depthRemaining > 3)
		depthRemaining--;

	bool FutileNode = staticScore + FutilityMargins[std::max<int>(0, depthRemaining)] < a;

	for (size_t i = 0; i < moves.size(); i++)	
	{
		if (moves[i] == hashMove)
			continue;

		position.ApplyMove(moves.at(i));
		tTable.PreFetch(position.GetZobristKey());							//load the transposition into l1 cache. ~5% speedup
		if (position.NodesSearchedAddToThreadTotal()) sharedData.AddNodeChunk();

		//futility pruning
		if (IsFutile(moves[i], beta, alpha, InCheck, position) && i > 0 && FutileNode)	//Possibly stop futility pruning if alpha or beta are close to mate scores
		{
			position.RevertMove();
			continue;
		}

		int extendedDepth = depthRemaining + extension(position, alpha, beta);

		//late move reductions
		if (LMR(InCheck, position) && i > 3)
		{
			int reduction = Reduction(depthRemaining, static_cast<int>(i), alpha, beta);
			int score = -NegaScout(position, initialDepth, extendedDepth - 1 - reduction, -a - 1, -a, -colour, distanceFromRoot + 1, true, locals, sharedData).GetScore();

			if (score <= a)
			{
				position.RevertMove();
				continue;
			}
		}

		int newScore = -NegaScout(position, initialDepth, extendedDepth - 1, -b, -a, -colour, distanceFromRoot + 1, true, locals, sharedData).GetScore();
		if (newScore > a && newScore < beta && i >= 1)
		{	
			newScore = -NegaScout(position, initialDepth, extendedDepth - 1, -beta, -a, -colour, distanceFromRoot + 1, true, locals, sharedData).GetScore();
		}

		position.RevertMove();

		UpdateScore(newScore, Score, bestMove, moves, i);
		UpdateAlpha(Score, a, moves, i, distanceFromRoot, locals);

		if (a >= beta) //Fail high cutoff
		{
			AddKiller(moves.at(i), distanceFromRoot, locals.KillerMoves);
			AddHistory(moves[i], depthRemaining, locals.HistoryMatrix, position.GetTurn());
			break;
		}

		b = a + 1;				//Set a new zero width window
	}

	if (!locals.AbortSearch(position.GetNodes()) && !sharedData.ThreadAbort(initialDepth))
		AddScoreToTable(Score, alpha, position, depthRemaining, distanceFromRoot, beta, bestMove);

	return SearchResult(Score, bestMove);
}

unsigned int ProbeTBRoot(const Position& position)
{
	return tb_probe_root(position.GetWhitePieces(), position.GetBlackPieces(),
		position.GetPieceBB(WHITE_KING) | position.GetPieceBB(BLACK_KING),
		position.GetPieceBB(WHITE_QUEEN) | position.GetPieceBB(BLACK_QUEEN),
		position.GetPieceBB(WHITE_ROOK) | position.GetPieceBB(BLACK_ROOK),
		position.GetPieceBB(WHITE_BISHOP) | position.GetPieceBB(BLACK_BISHOP),
		position.GetPieceBB(WHITE_KNIGHT) | position.GetPieceBB(BLACK_KNIGHT),
		position.GetPieceBB(WHITE_PAWN) | position.GetPieceBB(BLACK_PAWN),
		position.GetFiftyMoveCount(),
		position.CanCastleBlackKingside() * TB_CASTLING_k + position.CanCastleBlackQueenside() * TB_CASTLING_q + position.CanCastleWhiteKingside() * TB_CASTLING_K + position.CanCastleWhiteQueenside() * TB_CASTLING_Q,
		position.GetEnPassant() <= SQ_H8 ? position.GetEnPassant() : 0,
		position.GetTurn(),
		NULL);
}

unsigned int ProbeTBSearch(const Position& position)
{
	return tb_probe_wdl(position.GetWhitePieces(), position.GetBlackPieces(),
		position.GetPieceBB(WHITE_KING) | position.GetPieceBB(BLACK_KING),
		position.GetPieceBB(WHITE_QUEEN) | position.GetPieceBB(BLACK_QUEEN),
		position.GetPieceBB(WHITE_ROOK) | position.GetPieceBB(BLACK_ROOK),
		position.GetPieceBB(WHITE_BISHOP) | position.GetPieceBB(BLACK_BISHOP),
		position.GetPieceBB(WHITE_KNIGHT) | position.GetPieceBB(BLACK_KNIGHT),
		position.GetPieceBB(WHITE_PAWN) | position.GetPieceBB(BLACK_PAWN),
		0,
		position.CanCastleBlackKingside() * TB_CASTLING_k + position.CanCastleBlackQueenside() * TB_CASTLING_q + position.CanCastleWhiteKingside() * TB_CASTLING_K + position.CanCastleWhiteQueenside() * TB_CASTLING_Q,
		position.GetEnPassant() <= SQ_H8 ? position.GetEnPassant() : 0,
		position.GetTurn());
}

SearchResult UseSearchTBScore(unsigned int result, int staticEval)
{
	int score = -1;

	if (result == TB_LOSS)
		score = -5000 + staticEval / 10;
	else if (result == TB_BLESSED_LOSS)
		score = 0 + std::min(-1, staticEval / 100);
	else if (result == TB_DRAW)
		score = 0;
	else if (result == TB_CURSED_WIN)
		score = std::max(1, staticEval / 100);
	else if (result == TB_WIN)
		score = 5000 + staticEval / 10;
	else
		assert(0);

	return score;
}

SearchResult UseRootTBScore(unsigned int result, int staticEval)
{
	int score = -1;

	if (TB_GET_WDL(result) == TB_LOSS)
		score = -5000 + staticEval / 10;
	else if (TB_GET_WDL(result) == TB_BLESSED_LOSS)
		score = 0 + std::min(-1, staticEval / 100);
	else if (TB_GET_WDL(result) == TB_DRAW)
		score = 0;
	else if (TB_GET_WDL(result) == TB_CURSED_WIN)
		score = 0 + std::max(1, staticEval / 100);
	else if (TB_GET_WDL(result) == TB_WIN)
		score = 5000 + staticEval / 10;
	else
		assert(0);

	int flag = -1;

	if (TB_GET_PROMOTES(result) == TB_PROMOTES_NONE)
		flag = QUIET;
	else if (TB_GET_PROMOTES(result) == TB_PROMOTES_KNIGHT)
		flag = KNIGHT_PROMOTION;
	else if (TB_GET_PROMOTES(result) == TB_PROMOTES_BISHOP)
		flag = BISHOP_PROMOTION;
	else if (TB_GET_PROMOTES(result) == TB_PROMOTES_ROOK)
		flag = ROOK_PROMOTION;
	else if (TB_GET_PROMOTES(result) == TB_PROMOTES_QUEEN)
		flag = QUEEN_PROMOTION;
	else
		assert(0);

	Move move(TB_GET_FROM(result), TB_GET_TO(result), flag);

	return { score, move };
}

void UpdateAlpha(int Score, int& a, std::vector<Move>& moves, const size_t& i, unsigned int distanceFromRoot, SearchData& locals)
{
	if (Score > a)
	{
		a = Score;
		UpdatePV(moves.at(i), distanceFromRoot, locals.PvTable);
	}
}

void UpdateScore(int newScore, int& Score, Move& bestMove, std::vector<Move>& moves, const size_t& i)
{
	if (newScore > Score)
	{
		Score = newScore;
		bestMove = moves.at(i);
	}
}

int Reduction(int depth, int i, int alpha, int beta)
{
	/*Formula adapted from Fruit Reloaded, sourced from chess programming wiki*/
	if (IsPV(beta, alpha))
		return int((sqrt(static_cast<double>(depth - 1)) + sqrt(static_cast<double>(i - 1))) / 3);
	else
		return int((sqrt(static_cast<double>(depth - 1)) + sqrt(static_cast<double>(i - 1))) / 2);
}

void UpdatePV(Move move, int distanceFromRoot, std::vector<std::vector<Move>>& PvTable)
{
	PvTable[distanceFromRoot].clear();
	PvTable[distanceFromRoot].push_back(move);

	if (distanceFromRoot + 1 < static_cast<int>(PvTable.size()))
		PvTable[distanceFromRoot].insert(PvTable[distanceFromRoot].end(), PvTable[distanceFromRoot + 1].begin(), PvTable[distanceFromRoot + 1].end());
}

bool UseTransposition(TTEntry& entry, int distanceFromRoot, int alpha, int beta)
{
	entry.MateScoreAdjustment(distanceFromRoot);	//this MUST be done

	if (entry.GetCutoff() == EntryType::EXACT) return true;

	int NewAlpha = alpha;
	int NewBeta = beta;

	UpdateBounds(entry, NewAlpha, NewBeta);	//aspiration windows and search instability lead to issues with shrinking the original window

	if (NewAlpha >= NewBeta)
		return true;

	return false;
}

bool CheckForRep(Position& position, int distanceFromRoot)
{
	int totalRep = 1;
	uint64_t current = position.GetZobristKey();

	//note Previous keys will not contain the current key, hence rep starts at one
	for (size_t i = 0; i < position.GetPreviousKeysSize(); i++)
	{
		if (position.GetPreviousKey(i) == current)
		{
			totalRep++;
		}

		if (totalRep == 3) return true;																			//3 reps is always a draw
		if (totalRep == 2 && static_cast<int>(position.GetPreviousKeysSize() - i) < distanceFromRoot - 1) 
			return true;			//Don't allow 2 reps if its in the local search history (not part of the actual played game)
	}
	
	return false;
}

int extension(Position& position, int alpha, int beta)
{
	int extension = 0;

	if (IsPV(beta, alpha))
	{
		if (IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn()))	
			extension += 1;
	}

	return extension;
}

bool LMR(bool InCheck, const Position& position)
{
	return !InCheck
		&& !IsEndGame(position)
		&& !IsInCheck(position);
}

bool IsFutile(Move move, int beta, int alpha, bool InCheck, const Position& position)
{
	return !IsPV(beta, alpha)
		&& !move.IsCapture() 
		&& !move.IsPromotion() 
		&& !InCheck 
		&& !IsInCheck(position);
}

bool AllowedNull(bool allowedNull, const Position& position, int beta, int alpha)
{
	return allowedNull
		&& !IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn())
		&& !IsPV(beta, alpha)
		&& !IsEndGame(position)
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

void AddScoreToTable(int Score, int alphaOriginal, const Position& position, int depthRemaining, int distanceFromRoot, int beta, Move bestMove)
{
	if (Score <= alphaOriginal)
		tTable.AddEntry(bestMove, position.GetZobristKey(), Score, depthRemaining, position.GetTurnCount(), distanceFromRoot, EntryType::UPPERBOUND);	//mate score adjustent is done inside this function
	else if (Score >= beta)
		tTable.AddEntry(bestMove, position.GetZobristKey(), Score, depthRemaining, position.GetTurnCount(), distanceFromRoot, EntryType::LOWERBOUND);
	else
		tTable.AddEntry(bestMove, position.GetZobristKey(), Score, depthRemaining, position.GetTurnCount(), distanceFromRoot, EntryType::EXACT);
}

void UpdateBounds(const TTEntry& entry, int& alpha, int& beta)
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

int TerminalScore(const Position& position, int distanceFromRoot)
{
	if (IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn()))
	{
		return matedIn(distanceFromRoot);
	}
	else
	{
		return (Draw);
	}
}

int matedIn(int distanceFromRoot)
{
	return (MateScore) + (distanceFromRoot);
}

int mateIn(int distanceFromRoot)
{
	return -(MateScore) - (distanceFromRoot);
}

SearchResult Quiescence(Position& position, unsigned int initialDepth, int alpha, int beta, int colour, unsigned int distanceFromRoot, int depthRemaining, SearchData& locals, ThreadSharedData& sharedData)
{
	locals.PvTable[distanceFromRoot].clear();

	if (initialDepth > 1 && locals.AbortSearch(position.GetNodes())) return -1;
	if (sharedData.ThreadAbort(initialDepth)) return -1;									//another thread has finished searching this depth: ABORT!
	if (distanceFromRoot >= MAX_DEPTH) return 0;								//If we are 100 moves from root I think we can assume its a drawn position

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

	int staticScore = colour * EvaluatePositionNet(position, locals.evalTable);
	if (staticScore >= beta) return staticScore;
	if (staticScore > alpha) alpha = staticScore;
	
	Move bestmove;
	int Score = staticScore;

	QuiescenceMoves(position, moves);

	if (moves.size() == 0)
		return staticScore;
		
	OrderMoves(moves, position, distanceFromRoot, locals);

	for (size_t i = 0; i < moves.size(); i++)
	{
		int SEE = 0;
		if (moves[i].GetFlag() == CAPTURE) //seeCapture doesn't work for ep or promotions
		{
			SEE = seeCapture(position, moves[i]);
		}

		if (moves[i].IsPromotion())
		{
			SEE += PieceValues(WHITE_QUEEN);
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
		int newScore = -Quiescence(position, initialDepth, -beta, -alpha, -colour, distanceFromRoot + 1, depthRemaining - 1, locals, sharedData).GetScore();
		position.RevertMove();

		if (position.NodesSearchedAddToThreadTotal()) sharedData.AddNodeChunk();

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
	if (move.IsCapture() || move.IsPromotion() || move == KillerMoves.at(distanceFromRoot).move[0]) return;

	if (move == KillerMoves.at(distanceFromRoot).move[1])
	{
		std::swap(KillerMoves.at(distanceFromRoot).move[0], KillerMoves.at(distanceFromRoot).move[1]);
	}
	else
	{
		KillerMoves.at(distanceFromRoot).move[1] = move;	//replace the 2nd one
	}
}

void AddHistory(const Move& move, int depthRemaining, unsigned int(&HistoryMatrix)[N_PLAYERS][N_SQUARES][N_SQUARES], bool sideToMove)
{
	if (move.IsCapture() || move.IsPromotion()) return;
	HistoryMatrix[sideToMove][move.GetFrom()][move.GetTo()] += depthRemaining * depthRemaining;
}

Move GetHashMove(const Position& position, int depthRemaining, int distanceFromRoot)
{
	TTEntry hash = tTable.GetEntry(position.GetZobristKey());

	if (CheckEntry(hash, position.GetZobristKey(), depthRemaining))
	{
		tTable.SetNonAncient(position.GetZobristKey(), position.GetTurnCount(), distanceFromRoot);
		return hash.GetMove();
	}

	return {};
}

Move GetHashMove(const Position& position, int distanceFromRoot)
{
	TTEntry hash = tTable.GetEntry(position.GetZobristKey());

	if (CheckEntry(hash, position.GetZobristKey()))
	{
		tTable.SetNonAncient(position.GetZobristKey(), position.GetTurnCount(), distanceFromRoot);
		return hash.GetMove();
	}

	return {};
}

SearchData::SearchData() : HistoryMatrix{ {0} }
{
	PvTable.clear();
	for (unsigned int i = 0; i < MAX_DEPTH; i++)
	{
		PvTable.push_back(std::vector<Move>());
	}

	for (unsigned int i = 0; i < MAX_DEPTH; i++)
	{
		KillerMoves.push_back(Killer());
	}
}

bool SearchData::AbortSearch(size_t nodes)
{
	return timeManage.AbortSearch(nodes);
}

bool SearchData::ContinueSearch()
{
	return timeManage.ContinueSearch();
}

ThreadSharedData::ThreadSharedData(unsigned int threads, bool NoOutput) : currentBestMove()
{
	threadCount = threads;
	threadDepthCompleted = 0;
	prevScore = 0;
	noOutput = NoOutput;
	tbHits = 0;
	nodes = 0;

	for (unsigned int i = 0; i < threads; i++)
	{
		searchDepth.push_back(0);
		ThreadWantsToStop.push_back(false);
	}
}

ThreadSharedData::~ThreadSharedData()
{
}

Move ThreadSharedData::GetBestMove()
{
	std::lock_guard<std::mutex> lg(ioMutex);
	return currentBestMove;
}

bool ThreadSharedData::ThreadAbort(unsigned int initialDepth) const
{
	return initialDepth <= threadDepthCompleted;
}

void ThreadSharedData::ReportResult(unsigned int depth, double Time, int score, int alpha, int beta, const Position& position, Move move, const SearchData& locals)
{
	std::lock_guard<std::mutex> lg(ioMutex);

	if (alpha < score && score < beta && threadDepthCompleted < depth)
	{
		if (!noOutput)
			PrintSearchInfo(depth, Time, abs(score) > 9000, score, alpha, beta, position, move, locals, *this);

		threadDepthCompleted = depth;
		currentBestMove = move;
		prevScore = score;
	}
}

void ThreadSharedData::ReportDepth(unsigned int depth, unsigned int threadID)
{
	std::lock_guard<std::mutex> lg(ioMutex);
	searchDepth[threadID] = depth;
}

void ThreadSharedData::ReportWantsToStop(unsigned int threadID)
{
	std::lock_guard<std::mutex> lg(ioMutex);
	ThreadWantsToStop[threadID] = true;

	for (unsigned int i = 0; i < ThreadWantsToStop.size(); i++)
	{
		if (ThreadWantsToStop[i] == false)
			return;
	}

	KeepSearching = false;
}

int ThreadSharedData::GetAspirationScore()
{
	std::lock_guard<std::mutex> lg(ioMutex);
	return prevScore;
}
