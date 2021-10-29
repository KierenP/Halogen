#pragma once
#include "EvalCache.h"
#include "MoveList.h"
#include "Position.h"
#include "TimeManage.h"
#include "TranspositionTable.h"

extern TranspositionTable tTable;

class SearchLimits
{
public:
    bool CheckTimeLimit() const;
    bool CheckDepthLimit(int depth) const;
    bool CheckMateLimit(int score) const;
    bool CheckContinueSearch() const;

    void SetTimeLimits(int maxTime, int allocatedTime);
    void SetInfinite();
    void SetDepthLimit(int depth);
    void SetMateLimit(int moves);

    int ElapsedTime() const { return SearchTimeManager.ElapsedMs(); }

private:
    SearchTimeManage SearchTimeManager;
    mutable int PeriodicCheck = 0;

    int depthLimit = -1;
    int mateLimit = -1;
    bool IsInfinite = false;

    bool timeLimitEnabled = false;
    bool depthLimitEnabled = false;
    bool mateLimitEnabled = false;

    mutable bool periodicTimeLimit = false;
};

class History
{
public:
    void AddButterfly(const Position& position, Move move, int change);
    int16_t GetButterfly(const Position& position, Move move) const;

    void AddCounterMove(const Position& position, Move move, int change);
    int16_t GetCounterMove(const Position& position, Move move) const;

    // Tuneable history constants
    static int Butterfly_max;
    static int Butterfly_scale;

    static int CounterMove_max;
    static int CounterMove_scale;

private:
    void AddHistory(int16_t& val, int change, int max, int scale);

    // [side][from][to]
    using ButterflyType = std::array<std::array<std::array<int16_t, N_SQUARES>, N_SQUARES>, N_PLAYERS>;

    // [side][prev_piece][prev_to][piece][to]
    using CounterMoveType = std::array<std::array<std::array<std::array<std::array<int16_t, N_SQUARES>, N_PIECE_TYPES>, N_SQUARES>, N_PIECE_TYPES>, N_PLAYERS>;

    std::unique_ptr<ButterflyType> butterfly = std::make_unique<ButterflyType>();
    std::unique_ptr<CounterMoveType> counterMove = std::make_unique<CounterMoveType>();
};

inline int History::Butterfly_max = 16384;
inline int History::Butterfly_scale = 32;

inline int History::CounterMove_max = 16384;
inline int History::CounterMove_scale = 64;

struct SearchData
{
    explicit SearchData(const SearchLimits& Limits);

    //--------------------------------------------------------------------------------------------
private:
    uint64_t padding1[8] = {}; //To avoid false sharing between adjacent SearchData objects
    //--------------------------------------------------------------------------------------------

public:
    std::vector<std::vector<Move>> PvTable;
    std::vector<std::array<Move, 2>> KillerMoves; //2 moves indexed by distanceFromRoot

    EvalCacheTable evalTable;
    SearchLimits limits;

    void AddNode() { nodes++; }
    void AddTbHit() { tbHits++; }

    uint64_t GetThreadNodes() const { return nodes; }

    void ResetSeldepth() { selDepth = 0; }
    void ReportDepth(int distanceFromRoot) { selDepth = std::max(distanceFromRoot, selDepth); }
    int GetSelDepth() const { return selDepth; }

    History history;

private:
    friend class ThreadSharedData;
    uint64_t tbHits = 0;
    uint64_t nodes = 0;
    int selDepth = 0;

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
    ThreadSharedData(const SearchLimits& limits, const SearchParameters& parameters, bool NoOutput = false);

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

private:
    void PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, int score, int alpha, int beta, const Position& position, const Move& move, const SearchData& locals) const;
    bool MultiPVExcludeMoveUnlocked(Move move) const;

    mutable std::mutex ioMutex;
    unsigned int threadDepthCompleted = 0; //The depth that has been completed. When the first thread finishes a depth it increments this. All other threads should stop searching that depth
    Move currentBestMove = Move::Uninitialized; //Whoever finishes first gets to update this as long as they searched deeper than threadDepth
    int prevScore = 0; //if threads abandon the search, we need to know what the score was in order to set new alpha/beta bounds
    int lowestAlpha = 0;
    int highestBeta = 0;
    bool noOutput; //Do not write anything to the concole

    SearchParameters param;

    std::vector<SearchData> threadlocalData;
    std::vector<Move> MultiPVExclusion; //Moves that we ignore at the root for MPV mode

    //TODO: probably put this inside of SearchData
    std::vector<bool> ThreadWantsToStop; //Threads signal here that they want to stop searching, but will keep going until all threads want to stop
};
