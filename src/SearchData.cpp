#include "SearchData.h"

TranspositionTable tTable;

SearchData::SearchData(const SearchLimits& Limits) : limits(Limits)
{
	PvTable.resize(MAX_DEPTH);
	KillerMoves.resize(MAX_DEPTH);
}

ThreadSharedData::ThreadSharedData(const SearchLimits& limits, const SearchParameters& parameters, bool NoOutput) : param(parameters)
{
	noOutput = NoOutput;
	ThreadWantsToStop.resize(param.threads, false);
	threadlocalData.resize(param.threads, SearchData(limits));
}

Move ThreadSharedData::GetBestMove() const
{
	std::scoped_lock lock(ioMutex);
	return currentBestMove;
}

unsigned int ThreadSharedData::GetDepth() const
{
	std::scoped_lock lock (ioMutex);
	return threadDepthCompleted + 1;
}

bool ThreadSharedData::ThreadAbort(unsigned int initialDepth) const
{
	return initialDepth <= threadDepthCompleted;
}

void ThreadSharedData::ReportResult(unsigned int depth, double Time, int score, int alpha, int beta, const Position& position, Move move, const SearchData& locals)
{
	std::scoped_lock lock(ioMutex);

	if (alpha < score && score < beta && depth > threadDepthCompleted && !MultiPVExcludeMoveUnlocked(move))
	{
		if (!noOutput)
			PrintSearchInfo(depth, Time, abs(score) > TB_WIN_SCORE, score, alpha, beta, position, move, locals);

		if (GetMultiPVCount() == 0)
		{
			currentBestMove = move;
			prevScore = score;
			lowestAlpha = score;
			highestBeta = score;
		}

		MultiPVExclusion.push_back(move);

		if (GetMultiPVCount() >= GetMultiPVSetting())
		{
			threadDepthCompleted = depth;
			MultiPVExclusion.clear();
		}
	}

	if (score < lowestAlpha && score <= alpha && !noOutput && Time > 5000 && depth == threadDepthCompleted + 1)
	{
		PrintSearchInfo(depth, Time, abs(score) > TB_WIN_SCORE, score, alpha, beta, position, move, locals);
		lowestAlpha = alpha;
	}

	if (score > highestBeta && score >= beta && !noOutput && Time > 5000 && depth == threadDepthCompleted + 1)
	{
		PrintSearchInfo(depth, Time, abs(score) > TB_WIN_SCORE, score, alpha, beta, position, move, locals);
		highestBeta = beta;
	}
}

void ThreadSharedData::ReportWantsToStop(unsigned int threadID)
{
	std::scoped_lock lock(ioMutex);
	ThreadWantsToStop[threadID] = true;

	for (unsigned int i = 0; i < ThreadWantsToStop.size(); i++)
	{
		if (ThreadWantsToStop[i] == false)
			return;
	}

	KeepSearching = false;
}

int ThreadSharedData::GetAspirationScore() const
{
	std::scoped_lock lock(ioMutex);
	return prevScore;
}

int ThreadSharedData::GetMultiPVCount() const
{
	//can't lock, this is used during search
	return MultiPVExclusion.size();
}

bool ThreadSharedData::MultiPVExcludeMove(Move move) const
{
	std::scoped_lock lock(ioMutex);
	return std::find(MultiPVExclusion.begin(), MultiPVExclusion.end(), move) != MultiPVExclusion.end();
}

void ThreadSharedData::UpdateBestMove(Move move)
{
	std::scoped_lock lock(ioMutex);
	currentBestMove = move;
}

bool ThreadSharedData::MultiPVExcludeMoveUnlocked(Move move) const
{
	return std::find(MultiPVExclusion.begin(), MultiPVExclusion.end(), move) != MultiPVExclusion.end();
}

uint64_t ThreadSharedData::getTBHits() const
{
	return std::accumulate(threadlocalData.begin(), threadlocalData.end(), uint64_t(0),
		[](uint64_t sum, const SearchData& data) { return sum + data.tbHits; });
}

uint64_t ThreadSharedData::getNodes() const
{
	return std::accumulate(threadlocalData.begin(), threadlocalData.end(), uint64_t(0), 
		[](uint64_t sum, const SearchData& data) { return sum + data.nodes; });
}

SearchData& ThreadSharedData::GetData(unsigned int threadID)
{
	assert(threadID < threadlocalData.size());
	return threadlocalData[threadID];
}

void ThreadSharedData::PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, int score, int alpha, int beta, const Position& position, const Move& move, const SearchData& locals) const
{
	/*
	Here we avoid excessive use of std::cout and instead append to a string in order
	to output only once at the end. This causes a noticeable speedup for very fast
	time controls.
	*/

	std::vector<Move> pv = locals.PvTable[0];

	if (pv.size() == 0)
		pv.push_back(move);

	std::stringstream ss;

	ss	<< "info depth " << depth							//the depth of search
		<< " seldepth " << locals.GetSelDepth();			//the selective depth (for example searching further for checks and captures)

	if (isCheckmate)
	{
		if (score > 0)
			ss << " score mate " << ((MATE - abs(score)) + 1) / 2;
		else
			ss << " score mate " << -((MATE - abs(score)) + 1) / 2;
	}
	else
	{
		ss << " score cp " << score;						//The score in hundreths of a pawn (a 1 pawn advantage is +100)	
	}

	if (score <= alpha)
		ss << " upperbound";
	if (score >= beta)
		ss << " lowerbound";

	ss	<< " time " << Time																	//Time in ms
		<< " nodes " << getNodes()
		<< " nps " << int(getNodes() / std::max(int(Time), 1) * 1000)
		<< " hashfull " << tTable.GetCapacity(position.GetTurnCount())						//thousondths full
		<< " tbhits " << getTBHits();

	if (param.multiPV > 0)
		ss << " multipv " << GetMultiPVCount() + 1;

	ss << " pv ";														//the current best line found

	for (size_t i = 0; i < pv.size(); i++)
	{
		pv[i].Print(ss);
		ss << " ";
	}

	ss << "\n";
	std::cout << ss.str();
}

bool SearchLimits::CheckTimeLimit() const
{
	if (KeepSearching == false)
		return true;

	if (IsInfinite)
		return false;

	if (!timeLimitEnabled)
		return false;

	PeriodicCheck++;

	if (PeriodicCheck >= 1024)
	{
		PeriodicCheck = 0;
		periodicTimeLimit = SearchTimeManager.AbortSearch();
	}

	return periodicTimeLimit;
}

bool SearchLimits::CheckDepthLimit(int depth) const
{
	if (KeepSearching == false)
		return true;

	if (IsInfinite)
		return false;

	if (!depthLimitEnabled)
		return false;

	return depth > std::min(static_cast<int>(MAX_DEPTH), depthLimit);
}

bool SearchLimits::CheckMateLimit(int score) const
{
	if (KeepSearching == false)
		return true;

	if (IsInfinite)
		return false;

	if (!mateLimitEnabled)
		return false;

	return (MATE) - abs(score) <= 2 * mateLimit;
}

bool SearchLimits::CheckContinueSearch() const
{
	if (IsInfinite)
		return true;

	if (!timeLimitEnabled)
		return true;

	return SearchTimeManager.ContinueSearch();
}

void SearchLimits::SetTimeLimits(int maxTime, int allocatedTime)
{
	SearchTimeManager = SearchTimeManage(maxTime, allocatedTime);
	timeLimitEnabled = true;
}

void SearchLimits::SetInfinite()
{
	IsInfinite = true;
}

void SearchLimits::SetDepthLimit(int depth)
{
	depthLimit = depth;
	depthLimitEnabled = true;
}

void SearchLimits::SetMateLimit(int moves)
{
	mateLimit = moves;
	mateLimitEnabled = true;
}

void SearchData::AddHistory(int16_t& val, int change)
{
	val += 32 * change - val * abs(change) / 512;
}

int16_t& SearchData::Butterfly(const Position& position, Move move)
{
	assert(move != Move::Uninitialized);
	assert(ColourOfPiece(position.GetSquare(move.GetFrom())) == position.GetTurn());
	return history.Butterfly(position.GetTurn(), move.GetFrom(), move.GetTo());
}

int16_t SearchData::Butterfly(const Position& position, Move move) const
{
	assert(move != Move::Uninitialized);
	assert(ColourOfPiece(position.GetSquare(move.GetFrom())) == position.GetTurn());
	return history.Butterfly(position.GetTurn(), move.GetFrom(), move.GetTo());
}

History::History(const History& other) : 
	butterfly(std::make_unique<History::ButterflyType>(*other.butterfly))
{
}

History& History::operator=(const History& other)
{
	butterfly = std::make_unique<History::ButterflyType>(*other.butterfly);
	return *this;
}

int16_t& History::Butterfly(Players side, Square from, Square to)
{
	assert(side != N_PLAYERS);
	assert(from != N_SQUARES);
	assert(to != N_SQUARES);

	return (*butterfly)[side][from][to];
}

int16_t History::Butterfly(Players side, Square from, Square to) const
{
	assert(side != N_PLAYERS);
	assert(from != N_SQUARES);
	assert(to != N_SQUARES);

	return (*butterfly)[side][from][to];
}

