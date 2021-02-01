#include "Search.h"

/*Tuneable search constants*/

double LMR_constant = -1.76;
double LMR_coeff = 1.03;

int Null_constant = 4;
int Null_depth_quotent = 6;
int Null_beta_quotent = 250;

int Futility_linear = 25;
int Futility_constant = 100;

int Aspiration_window = 15;

int Delta_margin = 200;

int SNMP_depth = 7;
int SNMP_coeff = 119;

/*----------------*/

constexpr int FutilityMaxDepth = 10;
int FutilityMargins[FutilityMaxDepth];		//[depth]
int LMR_reduction[64][64] = {};				//[depth][move number]

void PrintBestMove(Move Best);
bool UseTransposition(TTEntry& entry, int alpha, int beta);
bool CheckForRep(const Position& position, int distanceFromRoot);
bool IsFutile(Move move, int beta, int alpha, Position & position, bool IsInCheck);
bool AllowedNull(bool allowedNull, const Position& position, int beta, int alpha, bool InCheck);
bool IsEndGame(const Position& position);
bool IsPV(int beta, int alpha);
void AddScoreToTable(int Score, int alphaOriginal, const Position& position, int depthRemaining, int distanceFromRoot, int beta, Move bestMove);
void UpdateBounds(const TTEntry& entry, int& alpha, int& beta);
int TerminalScore(const Position& position, int distanceFromRoot);
int extension(const Position & position, int alpha, int beta);
void AddKiller(Move move, int distanceFromRoot, std::vector<std::array<Move, 2>>& KillerMoves);
void AddHistory(const MoveGenerator& gen, const Move& move, SearchData& locals, int depthRemaining);
void UpdatePV(Move move, int distanceFromRoot, std::vector<std::vector<Move>>& PvTable);
int Reduction(int depth, int i);
int matedIn(int distanceFromRoot);
int mateIn(int distanceFromRoot);
int TBLossIn(int distanceFromRoot);
int TBWinIn(int distanceFromRoot);
unsigned int ProbeTBRoot(const Position& position);
unsigned int ProbeTBSearch(const Position& position);
SearchResult UseSearchTBScore(unsigned int result, int distanceFromRoot);
Move GetTBMove(unsigned int result);

void SearchPosition(Position position, ThreadSharedData& sharedData, unsigned int threadID);
SearchResult AspirationWindowSearch(Position& position, int depth, int prevScore, SearchData& locals, ThreadSharedData& sharedData, unsigned int threadID);
SearchResult NegaScout(Position& position, unsigned int initialDepth, int depthRemaining, int alpha, int beta, int colour, unsigned int distanceFromRoot, bool allowedNull, SearchData& locals, ThreadSharedData& sharedData);
void UpdateAlpha(int Score, int& a, const Move& move, unsigned int distanceFromRoot, SearchData& locals);
void UpdateScore(int newScore, int& Score, Move& bestMove, const Move& move);
SearchResult Quiescence(Position& position, unsigned int initialDepth, int alpha, int beta, int colour, unsigned int distanceFromRoot, int depthRemaining, SearchData& locals, ThreadSharedData& sharedData);

void InitSearch();

uint64_t SearchThread(const Position& position, const SearchParameters& parameters, const SearchLimits& limits, bool noOutput)
{
	//Probe TB at root
	if (GetBitCount(position.GetAllPieces()) <= TB_LARGEST)
	{
		unsigned int result = ProbeTBRoot(position);
		if (result != TB_RESULT_FAILED)
		{
			PrintBestMove(GetTBMove(result));
			return 0;
		}
	}

	InitSearch();

	std::vector<std::thread> threads;
	ThreadSharedData sharedData(limits, parameters, noOutput);

	for (int i = 0; i < parameters.threads; i++)
	{
		threads.emplace_back(std::thread([=, &sharedData] {SearchPosition(position, sharedData, i); }));
	}

	for (size_t i = 0; i < threads.size(); i++)
	{
		threads[i].join();
	}

	PrintBestMove(sharedData.GetBestMove());
	return sharedData.getNodes();				//Used by bench searches. Otherwise is discarded.
}

void InitSearch()
{
	KeepSearching = true;

	for (int i = 0; i < FutilityMaxDepth; i++)
	{
		FutilityMargins[i] = Futility_linear * i + Futility_constant;
	}

	for (int i = 0; i < 64; i++)
	{
		for (int j = 0; j < 64; j++)
		{
			LMR_reduction[i][j] = std::round(LMR_constant + LMR_coeff * log(i + 1) * log(j + 1));
		}
	}
}

void PrintBestMove(Move Best)
{
	std::cout << "bestmove ";
	Best.Print();
	std::cout << std::endl;
}

void SearchPosition(Position position, ThreadSharedData& sharedData, unsigned int threadID)
{
	int alpha = LowINF;
	int beta = HighINF;
	int prevScore = 0;

	while (sharedData.GetDepth() < MAX_DEPTH)
	{
		int depth = sharedData.GetDepth();

		if (sharedData.GetData(threadID).limits.CheckDepthLimit(depth)) break;
		if (!sharedData.GetData(threadID).limits.CheckContinueSearch())
			sharedData.ReportWantsToStop(threadID);

		SearchResult search = AspirationWindowSearch(position, depth, prevScore, sharedData.GetData(threadID), sharedData, threadID);
		int score = search.GetScore();

		if (depth > 1 && sharedData.GetData(threadID).limits.CheckTimeLimit()) break;
		
		if (sharedData.ThreadAbort(depth)) { score = sharedData.GetAspirationScore(); }

		sharedData.ReportResult(depth, sharedData.GetData(threadID).limits.ElapsedTime(), score, alpha, beta, position, search.GetMove(), sharedData.GetData(threadID));
		prevScore = score;

		if (sharedData.GetData(threadID).limits.CheckMateLimit(score)) break;
	}
}

SearchResult AspirationWindowSearch(Position& position, int depth, int prevScore, SearchData& locals, ThreadSharedData& sharedData, unsigned int threadID)
{
	int delta = Aspiration_window;

	int alpha = prevScore - delta;
	int beta  = prevScore + delta;
	SearchResult search = 0;

	while (true)
	{
		position.ResetSeldepth();
		search = NegaScout(position, depth, depth, alpha, beta, position.GetTurn() ? 1 : -1, 0, false, locals, sharedData);

		if (alpha < search.GetScore() && search.GetScore() < beta) break;
		if (sharedData.ThreadAbort(depth)) break;
		if (depth > 1 && sharedData.GetData(threadID).limits.CheckTimeLimit()) break;

		if (search.GetScore() <= alpha)
		{
			sharedData.ReportResult(depth, locals.limits.ElapsedTime(), alpha, alpha, beta, position, search.GetMove(), locals);
			beta = (alpha + beta) / 2;
			alpha = std::max<int>(MATED, alpha - delta);
		}

		if (search.GetScore() >= beta)
		{
			sharedData.ReportResult(depth, locals.limits.ElapsedTime(), beta, alpha, beta, position, search.GetMove(), locals);
			beta = std::min<int>(MATE, beta + delta);
		}

		delta = delta + delta / 2;
	} 

	return search;
}

SearchResult NegaScout(Position& position, unsigned int initialDepth, int depthRemaining, int alpha, int beta, int colour, unsigned int distanceFromRoot, bool allowedNull, SearchData& locals, ThreadSharedData& sharedData)
{
	position.ReportDepth(distanceFromRoot);

	if (distanceFromRoot >= MAX_DEPTH) return 0;						//Have we reached max depth?
	locals.PvTable[distanceFromRoot].clear();

	//See if we should abort the search
	if (initialDepth > 1 && locals.limits.CheckTimeLimit()) return -1;	//Am I out of time?
	if (sharedData.ThreadAbort(initialDepth)) return -1;				//Has this depth been finished by another thread?
	if (DeadPosition(position)) return 0;								//Is this position a dead draw?
	if (CheckForRep(position, distanceFromRoot)) return 0;				//Have we had a draw by repitition?
	if (position.GetFiftyMoveCount() > 100) return 0;					//cannot use >= as it could currently be checkmate which would count as a win
	
	int Score = LowINF;
	int MaxScore = HighINF;

	//Probe TB in search
	if (   distanceFromRoot > 0   
		&& position.GetFiftyMoveCount() == 0 
		&& GetBitCount(position.GetAllPieces()) <= TB_LARGEST)
	{
		unsigned int result = ProbeTBSearch(position);
		if (result != TB_RESULT_FAILED)
		{
			locals.AddTbHit();
			auto probe = UseSearchTBScore(result, distanceFromRoot);

			if (probe.GetScore() == 0)
				return probe;
			if (probe.GetScore() >= TBWinIn(MAX_DEPTH) && probe.GetScore() >= beta)
				return probe;
			if (probe.GetScore() <= TBLossIn(MAX_DEPTH) && probe.GetScore() <= alpha)
				return probe;

			if (IsPV(beta, alpha)) 
			{
				if (probe.GetScore() >= TBWinIn(MAX_DEPTH))
				{
					Score = probe.GetScore();
					alpha = std::max(alpha, probe.GetScore());
				}
				else
				{
					MaxScore = probe.GetScore();
				}
			}
		}
	}

	//Query the transpotition table
	if (!IsPV(beta, alpha)) 
	{
		TTEntry entry = tTable.GetEntry(position.GetZobristKey(), distanceFromRoot);
		if (CheckEntry(entry, position.GetZobristKey(), depthRemaining))
		{
			tTable.ResetAge(position.GetZobristKey(), position.GetTurnCount(), distanceFromRoot);

			if (!position.CheckForRep(distanceFromRoot, 2))	//Don't take scores from the TT if there's a two-fold repitition
				if (UseTransposition(entry, alpha, beta)) 
					return SearchResult(entry.GetScore(), entry.GetMove());
		}
	}

	bool InCheck = IsInCheck(position);

	//Drop into quiescence search
	if (depthRemaining <= 0 && !InCheck)
	{ 
		return Quiescence(position, initialDepth, alpha, beta, colour, distanceFromRoot, depthRemaining, locals, sharedData);
	}

	int staticScore = colour * EvaluatePositionNet(position, locals.evalTable); 

	//Static null move pruning
	if (depthRemaining <= SNMP_depth && staticScore - SNMP_coeff * depthRemaining >= beta && !InCheck && !IsPV(beta, alpha)) return beta;

	//Null move pruning
	if (AllowedNull(allowedNull, position, beta, alpha, InCheck) && (staticScore > beta))
	{
		unsigned int reduction = Null_constant + depthRemaining / Null_depth_quotent + std::min(3, (staticScore - beta) / Null_beta_quotent);

		position.ApplyNullMove();
		int score = -NegaScout(position, initialDepth, depthRemaining - reduction - 1, -beta, -beta + 1, -colour, distanceFromRoot + 1, false, locals, sharedData).GetScore();
		position.RevertNullMove();

		if (score >= beta)
		{
			if (beta < matedIn(MAX_DEPTH) || depthRemaining >= 10)	//TODO: I'm not sure about this first condition
			{
				//Do verification search for high depths
				SearchResult result = NegaScout(position, initialDepth, depthRemaining - reduction - 1, beta - 1, beta, colour, distanceFromRoot, false, locals, sharedData);
				if (result.GetScore() >= beta)
					return result;
			}
			else
			{
				return beta;
			}
		}
	}

	//mate distance pruning
	alpha = std::max<int>(matedIn(distanceFromRoot), alpha);
	beta = std::min<int>(mateIn(distanceFromRoot), beta);
	if (alpha >= beta)
		return alpha;

	//Set up search variables
	Move bestMove = Move();	
	int a = alpha;
	int b = beta;
	int searchedMoves;
	bool noLegalMoves = true;

	//Rebel style IID. Don't ask why this helps but it does.
	if (GetHashMove(position, distanceFromRoot).IsUninitialized() && depthRemaining > 3)
		depthRemaining--;

	bool FutileNode = (depthRemaining < FutilityMaxDepth) && (staticScore + FutilityMargins[std::max<int>(0, depthRemaining)] < a);

	MoveGenerator gen(position, distanceFromRoot, locals, false);
	Move move;

	for (searchedMoves = 0; gen.Next(move); searchedMoves++)
	{
		if (distanceFromRoot == 0 && sharedData.MultiPVExcludeMove(move))
			continue;

		noLegalMoves = false;
		locals.AddNode();

		//futility pruning
		if (IsFutile(move, beta, alpha, position, InCheck) && searchedMoves > 0 && FutileNode)	//Possibly stop futility pruning if alpha or beta are close to mate scores
			continue;

		position.ApplyMove(move);
		tTable.PreFetch(position.GetZobristKey());							//load the transposition into l1 cache. ~5% speedup
		int extendedDepth = depthRemaining + extension(position, alpha, beta);

		//late move reductions
		if (searchedMoves > 3)
		{
			int reduction = Reduction(depthRemaining, searchedMoves);

			if (IsPV(beta, alpha))
				reduction--;

			reduction = std::max(0, reduction);

			int score = -NegaScout(position, initialDepth, extendedDepth - 1 - reduction, -a - 1, -a, -colour, distanceFromRoot + 1, true, locals, sharedData).GetScore();

			if (score <= a)
			{
				position.RevertMove();
				continue;
			}
		}

		int newScore = -NegaScout(position, initialDepth, extendedDepth - 1, -b, -a, -colour, distanceFromRoot + 1, true, locals, sharedData).GetScore();
		if (newScore > a && newScore < beta && searchedMoves >= 1)	//MultiPV issues here
		{	
			newScore = -NegaScout(position, initialDepth, extendedDepth - 1, -beta, -a, -colour, distanceFromRoot + 1, true, locals, sharedData).GetScore();
		}

		position.RevertMove();

		UpdateScore(newScore, Score, bestMove, move);
		UpdateAlpha(Score, a, move, distanceFromRoot, locals);

		if (a >= beta) //Fail high cutoff
		{
			AddKiller(move, distanceFromRoot, locals.KillerMoves);
			AddHistory(gen, move, locals, depthRemaining);
			break;
		}

		b = a + 1;				//Set a new zero width window
	}

	//Checkmate or stalemate 
	if (noLegalMoves)
	{
		return TerminalScore(position, distanceFromRoot);
	}

	Score = std::min(Score, MaxScore);

	if (!locals.limits.CheckTimeLimit() && !sharedData.ThreadAbort(initialDepth))
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
		position.GetCanCastleBlackKingside() * TB_CASTLING_k + position.GetCanCastleBlackQueenside() * TB_CASTLING_q + position.GetCanCastleWhiteKingside() * TB_CASTLING_K + position.GetCanCastleWhiteQueenside() * TB_CASTLING_Q,
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
		position.GetFiftyMoveCount(),											
		position.GetCanCastleBlackKingside() * TB_CASTLING_k + position.GetCanCastleBlackQueenside() * TB_CASTLING_q + position.GetCanCastleWhiteKingside() * TB_CASTLING_K + position.GetCanCastleWhiteQueenside() * TB_CASTLING_Q,
		position.GetEnPassant() <= SQ_H8 ? position.GetEnPassant() : 0,
		position.GetTurn());
}

SearchResult UseSearchTBScore(unsigned int result, int distanceFromRoot)
{
	int score = -1;

	if (result == TB_LOSS)
		score = -5000 + distanceFromRoot;
	else if (result == TB_BLESSED_LOSS)
		score = 0;
	else if (result == TB_DRAW)
		score = 0;
	else if (result == TB_CURSED_WIN)
		score = 0;
	else if (result == TB_WIN)
		score = 5000 - distanceFromRoot;
	else
		assert(0);

	return score;
}

Move GetTBMove(unsigned int result)
{
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

	return Move(static_cast<Square>(TB_GET_FROM(result)), static_cast<Square>(TB_GET_TO(result)), static_cast<MoveFlag>(flag));
}

void UpdateAlpha(int Score, int& a, const Move& move, unsigned int distanceFromRoot, SearchData& locals)
{
	if (Score > a)
	{
		a = Score;
		UpdatePV(move, distanceFromRoot, locals.PvTable);
	}
}

void UpdateScore(int newScore, int& Score, Move& bestMove, const Move& move)
{
	if (newScore > Score)
	{
		Score = newScore;
		bestMove = move;
	}
}

int Reduction(int depth, int i)
{
	return LMR_reduction[std::min(63, std::max(0, depth))][std::min(63, std::max(0, i))];
}

void UpdatePV(Move move, int distanceFromRoot, std::vector<std::vector<Move>>& PvTable)
{
	PvTable[distanceFromRoot].clear();
	PvTable[distanceFromRoot].push_back(move);

	if (distanceFromRoot + 1 < static_cast<int>(PvTable.size()))
		PvTable[distanceFromRoot].insert(PvTable[distanceFromRoot].end(), PvTable[distanceFromRoot + 1].begin(), PvTable[distanceFromRoot + 1].end());
}

bool UseTransposition(TTEntry& entry, int alpha, int beta)
{
	if (entry.GetCutoff() == EntryType::EXACT) return true;

	int NewAlpha = alpha;
	int NewBeta = beta;

	UpdateBounds(entry, NewAlpha, NewBeta);	//aspiration windows and search instability lead to issues with shrinking the original window

	if (NewAlpha >= NewBeta)
		return true;

	return false;
}

bool CheckForRep(const Position& position, int distanceFromRoot)
{
	return position.CheckForRep(distanceFromRoot, 3);
}

int extension(const Position& position, int alpha, int beta)
{
	int extension = 0;

	if (IsPV(beta, alpha))
	{
		if (IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn()))	
			extension += 1;
	}

	return extension;
}

bool FutilityMoveGivesCheck(Position& position, Move move)
{
	position.ApplyMoveQuick(move);
	bool ret = IsInCheck(position, !position.GetTurn());	//ApplyMoveQuick does not change whos turn it is
	position.RevertMoveQuick();
	return ret;
}

bool IsFutile(Move move, int beta, int alpha, Position & position, bool IsInCheck)
{
	return !IsPV(beta, alpha)
		&& !move.IsCapture() 
		&& !move.IsPromotion() 
		&& !IsInCheck
		&& !FutilityMoveGivesCheck(position, move);
}

bool AllowedNull(bool allowedNull, const Position& position, int beta, int alpha, bool InCheck)
{
	return allowedNull
		&& !InCheck
		&& !IsPV(beta, alpha)
		&& !IsEndGame(position)
		&& GetBitCount(position.GetAllPieces()) >= 5;	//avoid null move pruning in very late game positions due to zanauag issues. Even with verification search e.g 8/6k1/8/8/8/8/1K6/Q7 w - - 0 1 
}

bool IsEndGame(const Position& position)
{
	return (position.GetPiecesColour(position.GetTurn()) == (position.GetPieceBB(KING, position.GetTurn()) | position.GetPieceBB(PAWN, position.GetTurn())));
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
		return (DRAW);
	}
}

int matedIn(int distanceFromRoot)
{
	return MATED + distanceFromRoot;
}

int mateIn(int distanceFromRoot)
{
	return MATE - distanceFromRoot;
}

int TBLossIn(int distanceFromRoot)
{
	return TB_LOSS_SCORE + distanceFromRoot;
}

int TBWinIn(int distanceFromRoot)
{
	return TB_WIN_SCORE - distanceFromRoot;
}

SearchResult Quiescence(Position& position, unsigned int initialDepth, int alpha, int beta, int colour, unsigned int distanceFromRoot, int depthRemaining, SearchData& locals, ThreadSharedData& sharedData)
{
	position.ReportDepth(distanceFromRoot);

	if (distanceFromRoot >= MAX_DEPTH) return 0;						//Have we reached max depth?
	locals.PvTable[distanceFromRoot].clear();

	//See if we should abort the search
	if (initialDepth > 1 && locals.limits.CheckTimeLimit()) return -1;	//Am I out of time?
	if (sharedData.ThreadAbort(initialDepth)) return -1;				//Has this depth been finished by another thread?
	if (DeadPosition(position)) return 0;								//Is this position a dead draw?

	int staticScore = colour * EvaluatePositionNet(position, locals.evalTable);
	if (staticScore >= beta) return staticScore;
	if (staticScore > alpha) alpha = staticScore;
	
	Move bestmove;
	int Score = staticScore;

	MoveGenerator gen(position, distanceFromRoot, locals, true);
	Move move;

	while (gen.Next(move))
	{
		locals.AddNode();

		int SEE = gen.GetSEE();

		if (move.IsPromotion())
		{
			SEE = PieceValues(WHITE_QUEEN);
		}

		if (staticScore + SEE + Delta_margin < alpha) 						//delta pruning
			break;

		if (SEE < 0)														//prune bad captures
			break;

		if (SEE <= 0 && position.GetCaptureSquare() != move.GetTo())	//prune equal captures that aren't recaptures
			continue;

		if (move.IsPromotion() && !(move.GetFlag() == QUEEN_PROMOTION || move.GetFlag() == QUEEN_PROMOTION_CAPTURE))	//prune underpromotions
			continue;

		position.ApplyMove(move);
		int newScore = -Quiescence(position, initialDepth, -beta, -alpha, -colour, distanceFromRoot + 1, depthRemaining - 1, locals, sharedData).GetScore();
		position.RevertMove();

		UpdateScore(newScore, Score, bestmove, move);
		UpdateAlpha(Score, alpha, move, distanceFromRoot, locals);

		if (Score >= beta)
			break;
	}

	return SearchResult(Score, bestmove);
}

void AddKiller(Move move, int distanceFromRoot, std::vector<std::array<Move, 2>>& KillerMoves)
{
	if (move.IsCapture() || move.IsPromotion() || move == KillerMoves[distanceFromRoot][0]) return;

	if (move == KillerMoves[distanceFromRoot][1])
	{
		std::swap(KillerMoves[distanceFromRoot][0], KillerMoves[distanceFromRoot][1]);
	}
	else
	{
		KillerMoves[distanceFromRoot][1] = move;	//replace the 2nd one
	}
}

void AddHistory(const MoveGenerator& gen, const Move& move, SearchData& locals, int depthRemaining)
{
	if (depthRemaining > 20 || move.IsCapture() || move.IsPromotion()) return;
	gen.AdjustHistory(move, locals, depthRemaining);
}

