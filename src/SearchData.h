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

    int ElapsedTime() const { return TimeManager.ElapsedMs(); }

    void Reset() { TimeManager.Reset(); }

private:
    SearchTimeManage TimeManager;
    int depthLimit = -1;
    int mateLimit = -1;
    bool IsInfinite = false;

    bool timeLimitEnabled = false;
    bool depthLimitEnabled = false;
    bool mateLimitEnabled = false;
};

class History
{
public:
    History() { Reset(); }

    void AddButterfly(const Position& position, Move move, int change);
    int16_t GetButterfly(const Position& position, Move move) const;

    void AddCounterMove(const Position& position, Move move, int change);
    int16_t GetCounterMove(const Position& position, Move move) const;

    // Tuneable history constants
    static int Butterfly_max;
    static int Butterfly_scale;

    static int CounterMove_max;
    static int CounterMove_scale;

    void Reset();

private:
    void AddHistory(int16_t& val, int change, int max, int scale);

    // [side][from][to]
    using ButterflyType = std::array<std::array<std::array<int16_t, N_SQUARES>, N_SQUARES>, N_PLAYERS>;

    // [side][prev_piece][prev_to][piece][to]
    using CounterMoveType = std::array<std::array<std::array<std::array<std::array<int16_t, N_SQUARES>, N_PIECE_TYPES>, N_SQUARES>, N_PIECE_TYPES>, N_PLAYERS>;

    std::unique_ptr<ButterflyType> butterfly;
    std::unique_ptr<CounterMoveType> counterMove;
};

inline int History::Butterfly_max = 16384;
inline int History::Butterfly_scale = 32;

inline int History::CounterMove_max = 16384;
inline int History::CounterMove_scale = 64;

struct SearchData
{
    explicit SearchData(SearchLimits* Limits);

    //--------------------------------------------------------------------------------------------
private:
    uint64_t padding1[8] = {}; //To avoid false sharing between adjacent SearchData objects
    //--------------------------------------------------------------------------------------------

public:
    std::array<BasicMoveList, MAX_DEPTH> PvTable = {};
    std::array<std::array<Move, 2>, MAX_DEPTH> KillerMoves = {}; //2 moves indexed by distanceFromRoot

    EvalCacheTable evalTable;
    History history;
    SearchLimits* limits;

    void AddNode() { nodes++; }
    void AddTbHit() { tbHits++; }

    uint64_t GetThreadNodes() const { return nodes; }

    void ResetSeldepth() { selDepth = 0; }
    void ReportDepth(int distanceFromRoot) { selDepth = std::max(distanceFromRoot, selDepth); }
    int GetSelDepth() const { return selDepth; }

    void Reset();

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
    ThreadSharedData(const SearchLimits& limits = {}, const SearchParameters& parameters = {});

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

    void SetLimits(const SearchLimits& limits);
    void SetMultiPv(int multiPv) { param.multiPV = multiPv; }
    void SetThreads(int threads);
    const SearchParameters& GetParameters() { return param; }

    void Reset();

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
