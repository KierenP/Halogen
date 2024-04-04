#include "SearchData.h"

#include <assert.h>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdlib.h>
#include <string>

#include "GameState.h"
#include "Zobrist.h"

TranspositionTable tTable;

void History::Reset()
{
    butterfly = std::make_unique<ButterflyType>();
    counterMove = std::make_unique<CounterMoveType>();
}

SearchData::SearchData()
{
    ResetNewGame();
}

void SearchData::ResetNewSearch()
{
    tbHits = 0;
    nodes = 0;
    selDepth = 0;
    threadWantsToStop = false;

    PvTable = {};
    KillerMoves = {};
}

void SearchData::ResetNewGame()
{
    ResetNewSearch();
    history.Reset();
}

ThreadSharedData::ThreadSharedData(SearchLimits limits, const SearchParameters& parameters)
{
    ResetNewGame();
    SetLimits(std::move(limits));
    SetMultiPv(parameters.multiPV);
    SetThreads(parameters.threads);
}

void ThreadSharedData::SetLimits(SearchLimits limits)
{
    limits_ = std::move(limits);
}

void ThreadSharedData::SetThreads(int threads)
{
    param.threads = threads;
    threadlocalData.resize(param.threads);
}

void ThreadSharedData::ResetNewSearch()
{
    threadDepthCompleted = 0;
    currentBestMove = Move::Uninitialized;
    prevScore = 0;
    lowestAlpha = 0;
    highestBeta = 0;

    limits_.ResetTimer();
    std::for_each(threadlocalData.begin(), threadlocalData.end(), [](auto& data) { data.ResetNewSearch(); });
    MultiPVExclusion.clear();
}

void ThreadSharedData::ResetNewGame()
{
    threadDepthCompleted = 0;
    currentBestMove = Move::Uninitialized;
    prevScore = 0;
    lowestAlpha = 0;
    highestBeta = 0;

    limits_.ResetTimer();
    std::for_each(threadlocalData.begin(), threadlocalData.end(), [](auto& data) { data.ResetNewGame(); });
    MultiPVExclusion.clear();
}

Move ThreadSharedData::GetBestMove() const
{
    std::scoped_lock lock(ioMutex);
    return currentBestMove;
}

unsigned int ThreadSharedData::GetDepth() const
{
    std::scoped_lock lock(ioMutex);
    return threadDepthCompleted + 1;
}

bool ThreadSharedData::ThreadAbort(unsigned int initialDepth) const
{
    return initialDepth <= threadDepthCompleted;
}

void ThreadSharedData::ReportResult(unsigned int depth, double Time, int score, int alpha, int beta,
    GameState& position, Move move, const SearchData& locals, bool chess960)
{
    std::scoped_lock lock(ioMutex);

    if (alpha < score && score < beta && depth > threadDepthCompleted && !MultiPVExcludeMoveUnlocked(move))
    {
        PrintSearchInfo(depth, Time, abs(score) > TB_WIN_SCORE, score, alpha, beta, position, locals, chess960);

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

    if (score < lowestAlpha && score <= alpha && Time > 5000 && depth == threadDepthCompleted + 1)
    {
        PrintSearchInfo(depth, Time, abs(score) > TB_WIN_SCORE, score, alpha, beta, position, locals, chess960);
        lowestAlpha = alpha;
    }

    if (score > highestBeta && score >= beta && Time > 5000 && depth == threadDepthCompleted + 1)
    {
        PrintSearchInfo(depth, Time, abs(score) > TB_WIN_SCORE, score, alpha, beta, position, locals, chess960);
        highestBeta = beta;
    }
}

void ThreadSharedData::ReportWantsToStop(unsigned int threadID)
{
    std::scoped_lock lock(ioMutex);
    threadlocalData[threadID].threadWantsToStop = true;

    for (unsigned int i = 0; i < threadlocalData.size(); i++)
    {
        if (threadlocalData[i].threadWantsToStop == false)
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
    // can't lock, this is used during search
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

void ThreadSharedData::PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, int score, int alpha,
    int beta, GameState& position, const SearchData& locals, bool chess960) const
{
    /*
        Here we avoid excessive use of std::cout and instead append to a string in order
        to output only once at the end. This causes a noticeable speedup for very fast
        time controls.
        */

    const BasicMoveList& pv = locals.PvTable[0];

    std::stringstream ss;

    ss << "info depth " << depth // the depth of search
       << " seldepth "
       << locals.GetSelDepth(); // the selective depth (for example searching further for checks and captures)

    if (isCheckmate)
    {
        if (score > 0)
            ss << " score mate " << ((MATE - abs(score)) + 1) / 2;
        else
            ss << " score mate " << -((MATE - abs(score)) + 1) / 2;
    }
    else
    {
        ss << " score cp " << score; // The score in hundreths of a pawn (a 1 pawn advantage is +100)
    }

    if (score <= alpha)
        ss << " upperbound";
    if (score >= beta)
        ss << " lowerbound";

    ss << " time " << Time // Time in ms
       << " nodes " << getNodes() << " nps " << int(getNodes() / std::max(int(Time), 1) * 1000) << " hashfull "
       << tTable.GetCapacity(position.Board().half_turn_count) // thousondths full
       << " tbhits " << getTBHits();

    if (param.multiPV > 0)
        ss << " multipv " << GetMultiPVCount() + 1;

    ss << " pv "; // the current best line found

    for (size_t i = 0; i < pv.size(); i++)
    {
        if (chess960)
        {
            ss << pv[i].to_string_960(position.Board().stm, position.Board().castle_squares);
        }
        else
        {
            ss << pv[i].to_string();
        }

        // for chess960 positions to be printed correctly, we must have the correct board state at time of printing
        position.ApplyMove(pv[i]);

        ss << " ";
    }

    for (size_t i = 0; i < pv.size(); i++)
    {
        position.RevertMove();
    }

    std::cout << ss.str() << std::endl;
}

void History::Add(const GameState& position, Move move, int change)
{
    AddButterfly(position, move, change);
    AddCounterMove(position, move, change);
}

int History::Get(const GameState& position, Move move) const
{
    return GetButterfly(position, move) + GetCounterMove(position, move);
}

void History::AddHistory(int16_t& val, int change, int max, int scale)
{
    val += scale * change - val * abs(change) * scale / max;
}

void History::AddButterfly(const GameState& position, Move move, int change)
{
    assert(move != Move::Uninitialized);

    assert(ColourOfPiece(position.Board().GetSquare(move.GetFrom())) == position.Board().stm);
    assert(position.Board().stm != N_PLAYERS);
    assert(move.GetFrom() != N_SQUARES);
    assert(move.GetTo() != N_SQUARES);

    AddHistory(
        (*butterfly)[position.Board().stm][move.GetFrom()][move.GetTo()], change, Butterfly_max, Butterfly_scale);
}

int16_t History::GetButterfly(const GameState& position, Move move) const
{
    assert(move != Move::Uninitialized);

    assert(ColourOfPiece(position.Board().GetSquare(move.GetFrom())) == position.Board().stm);
    assert(position.Board().stm != N_PLAYERS);
    assert(move.GetFrom() != N_SQUARES);
    assert(move.GetTo() != N_SQUARES);

    return (*butterfly)[position.Board().stm][move.GetFrom()][move.GetTo()];
}

void History::AddCounterMove(const GameState& position, Move move, int change)
{
    Move prevMove = position.GetPreviousMove();
    if (prevMove == Move::Uninitialized)
        return;

    assert(move != Move::Uninitialized);

    PieceTypes prevPiece = GetPieceType(position.Board().GetSquare(prevMove.GetTo()));
    PieceTypes currentPiece = GetPieceType(position.Board().GetSquare(move.GetFrom()));

    assert(prevPiece != N_PIECE_TYPES);
    assert(currentPiece != N_PIECE_TYPES);
    assert(prevMove.GetTo() != N_SQUARES);
    assert(move.GetTo() != N_SQUARES);

    AddHistory((*counterMove)[position.Board().stm][prevPiece][prevMove.GetTo()][currentPiece][move.GetTo()], change,
        CounterMove_max, CounterMove_scale);
}

int16_t History::GetCounterMove(const GameState& position, Move move) const
{
    Move prevMove = position.GetPreviousMove();
    if (prevMove == Move::Uninitialized)
        return 0;

    assert(move != Move::Uninitialized);

    PieceTypes prevPiece = GetPieceType(position.Board().GetSquare(prevMove.GetTo()));
    PieceTypes currentPiece = GetPieceType(position.Board().GetSquare(move.GetFrom()));

    assert(prevPiece != N_PIECE_TYPES);
    assert(currentPiece != N_PIECE_TYPES);
    assert(prevMove.GetTo() != N_SQUARES);
    assert(move.GetTo() != N_SQUARES);

    return (*counterMove)[position.Board().stm][prevPiece][prevMove.GetTo()][currentPiece][move.GetTo()];
}