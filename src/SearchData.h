#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "BitBoardDefine.h"
#include "EvalCache.h"
#include "Move.h"
#include "MoveList.h"
#include "SearchLimits.h"
#include "TimeManage.h"
#include "TranspositionTable.h"

class Position;

extern TranspositionTable tTable;

class History
{
public:
    History() { Reset(); }
    void Reset();

    void Add(const Position& position, Move move, int change);
    int Get(const Position& position, Move move) const;

private:
    // Tuneable history constants
    static constexpr int Butterfly_max = 16384;
    static constexpr int Butterfly_scale = 32;

    static constexpr int CounterMove_max = 16384;
    static constexpr int CounterMove_scale = 64;

    void AddButterfly(const Position& position, Move move, int change);
    int16_t GetButterfly(const Position& position, Move move) const;

    void AddCounterMove(const Position& position, Move move, int change);
    int16_t GetCounterMove(const Position& position, Move move) const;

    void AddHistory(int16_t& val, int change, int max, int scale);

    // [side][from][to]
    using ButterflyType = std::array<std::array<std::array<int16_t, N_SQUARES>, N_SQUARES>, N_PLAYERS>;

    // [side][prev_piece][prev_to][piece][to]
    using CounterMoveType = std::array<std::array<std::array<std::array<std::array<int16_t, N_SQUARES>, N_PIECE_TYPES>, N_SQUARES>, N_PIECE_TYPES>, N_PLAYERS>;

    std::unique_ptr<ButterflyType> butterfly;
    std::unique_ptr<CounterMoveType> counterMove;
};

struct SearchData
{
    explicit SearchData();

    //--------------------------------------------------------------------------------------------
private:
    uint64_t padding1[8] = {}; //To avoid false sharing between adjacent SearchData objects
    //--------------------------------------------------------------------------------------------

public:
    std::array<BasicMoveList, MAX_DEPTH> PvTable = {};
    std::array<std::array<Move, 2>, MAX_DEPTH> KillerMoves = {}; //2 moves indexed by distanceFromRoot

    EvalCacheTable evalTable;
    History history;

    void AddNode() { nodes++; }
    void AddTbHit() { tbHits++; }

    uint64_t GetThreadNodes() const { return nodes; }

    void ResetSeldepth() { selDepth = 0; }
    void ReportDepth(int distanceFromRoot) { selDepth = std::max(distanceFromRoot, selDepth); }
    int GetSelDepth() const { return selDepth; }

    void ResetNewSearch();
    void ResetNewGame();

private:
    friend class ThreadSharedData;
    uint64_t tbHits;
    uint64_t nodes;
    int selDepth;
    bool threadWantsToStop;

    //--------------------------------------------------------------------------------------------
private:
    uint64_t padding2[8] = {}; //To avoid false sharing between adjacent SearchData objects
    //--------------------------------------------------------------------------------------------
};

struct SearchParameters
{
    int threads = 1;
    int multiPV = 0;
};

class ThreadSharedData
{
public:
    ThreadSharedData(SearchLimits limits = {}, const SearchParameters& parameters = {});

    Move GetBestMove() const;
    unsigned int GetDepth() const;
    bool ThreadAbort(unsigned int initialDepth) const;
    void ReportResult(unsigned int depth, double Time, int score, int alpha, int beta, const Position& position, Move move, const SearchData& locals);
    void ReportWantsToStop(unsigned int threadID);
    int GetAspirationScore() const;
    int GetMultiPVSetting() const { return param.multiPV; };
    int GetMultiPVCount() const;
    bool MultiPVExcludeMove(Move move) const;
    void UpdateBestMove(Move move);

    uint64_t getTBHits() const;
    uint64_t getNodes() const;

    SearchData& GetData(unsigned int threadID);

    void SetLimits(SearchLimits limits);
    void SetMultiPv(int multiPv) { param.multiPV = multiPv; }
    void SetThreads(int threads);
    const SearchParameters& GetParameters() { return param; }
    const SearchLimits& GetLimits() { return limits_; }

    void ResetNewSearch();
    void ResetNewGame();

private:
    void PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, int score, int alpha, int beta, const Position& position, const SearchData& locals) const;
    bool MultiPVExcludeMoveUnlocked(Move move) const;

    mutable std::mutex ioMutex;
    unsigned int threadDepthCompleted; //The depth that has been completed. When the first thread finishes a depth it increments this. All other threads should stop searching that depth
    Move currentBestMove; //Whoever finishes first gets to update this as long as they searched deeper than threadDepth
    int prevScore; //if threads abandon the search, we need to know what the score was in order to set new alpha/beta bounds
    int lowestAlpha;
    int highestBeta;

    SearchParameters param;
    SearchLimits limits_;

    std::vector<SearchData> threadlocalData;
    std::vector<Move> MultiPVExclusion; //Moves that we ignore at the root for MPV mode
};
