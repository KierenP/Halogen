#include "SearchData.h"

TranspositionTable tTable;

SearchData::SearchData(const SearchLimits& Limits) : HistoryMatrix {}, limits(Limits)
{
	PvTable.resize(MAX_DEPTH);
	KillerMoves.resize(MAX_DEPTH);
}

ThreadSharedData::ThreadSharedData(const SearchLimits& limits, unsigned int threads, bool NoOutput) : currentBestMove()
{
	threadCount = threads;
	threadDepthCompleted = 0;
	prevScore = 0;
	noOutput = NoOutput;
	lowestAlpha = 0;
	highestBeta = 0;

	for (unsigned int i = 0; i < threads; i++)
	{
		searchDepth.push_back(0);
		ThreadWantsToStop.push_back(false);
		threadlocalData.emplace_back(limits);
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
			PrintSearchInfo(depth, Time, abs(score) > 9000, score, alpha, beta, position, move, locals);

		threadDepthCompleted = depth;
		currentBestMove = move;
		prevScore = score;
		lowestAlpha = score;
		highestBeta = score;
	}

	if (score < lowestAlpha && score <= alpha && !noOutput && Time > 5000 && threadDepthCompleted == depth - 1)
	{
		PrintSearchInfo(depth, Time, abs(score) > 9000, score, alpha, beta, position, move, locals);
		lowestAlpha = alpha;
	}

	if (score > highestBeta && score >= beta && !noOutput && Time > 5000 && threadDepthCompleted == depth - 1)
	{
		PrintSearchInfo(depth, Time, abs(score) > 9000, score, alpha, beta, position, move, locals);
		highestBeta = beta;
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

uint64_t ThreadSharedData::getTBHits() const
{
	uint64_t ret = 0;

	for (auto& x : threadlocalData)
		ret += x.tbHits;

	return ret;
}

uint64_t ThreadSharedData::getNodes() const
{
	uint64_t ret = 0;

	for (auto& x : threadlocalData)
		ret += x.nodes;

	return ret;
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
		<< " seldepth " << position.GetSelDepth();			//the selective depth (for example searching further for checks and captures)

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
