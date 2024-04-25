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
#include "Search.h"
#include "SearchLimits.h"
#include "TimeManage.h"
#include "TranspositionTable.h"
#include "Zobrist.h"

extern TranspositionTable tTable;

class GameState;

// Holds information about the search state for a particular recursion depth.
struct SearchStackState
{
    BasicMoveList pv = {};
    std::array<Move, 2> killers = {};

    Move move = Move::Uninitialized;
    Move singular_exclusion = Move::Uninitialized;
    int multiple_extensions = 0;
};

class SearchStack
{
public:
    // The search accesses [-1, MAX_DEPTH], so the root is [1] and we have MAX_DEPTH+2 elements
    SearchStackState* root()
    {
        return &search_stack_array_[1];
    }

private:
    std::array<SearchStackState, MAX_DEPTH + 2> search_stack_array_ = {};
};

class History
{
public:
    History()
    {
        Reset();
    }

    void Reset();

    void Add(const GameState& position, const SearchStackState* ss, Move move, int change);
    int Get(const GameState& position, const SearchStackState* ss, Move move) const;

private:
    // Tuneable history constants
    static constexpr int Butterfly_max = 16384;
    static constexpr int Butterfly_scale = 32;

    static constexpr int CounterMove_max = 16384;
    static constexpr int CounterMove_scale = 64;

    void AddButterfly(const GameState& position, Move move, int change);
    int16_t GetButterfly(const GameState& position, Move move) const;

    void AddCounterMove(const GameState& position, const SearchStackState* ss, Move move, int change);
    int16_t GetCounterMove(const GameState& position, const SearchStackState* ss, Move move) const;

    void AddHistory(int16_t& val, int change, int max, int scale);

    // [side][from][to]
    using ButterflyType = std::array<std::array<std::array<int16_t, N_SQUARES>, N_SQUARES>, N_PLAYERS>;

    // [side][prev_piece][prev_to][piece][to]
    using CounterMoveType = std::array<
        std::array<std::array<std::array<std::array<int16_t, N_SQUARES>, N_PIECE_TYPES>, N_SQUARES>, N_PIECE_TYPES>,
        N_PLAYERS>;

    std::unique_ptr<ButterflyType> butterfly;
    std::unique_ptr<CounterMoveType> counterMove;
};

// Data local to a particular thread
struct SearchLocalState
{
    //--------------------------------------------------------------------------------------------
private:
    uint64_t padding1[8] = {}; // To avoid false sharing between adjacent SearchData objects
    //--------------------------------------------------------------------------------------------

public:
    SearchStack search_stack;

    EvalCacheTable eval_cache;
    History history;

    std::atomic<uint64_t> tb_hits = 0;
    std::atomic<uint64_t> nodes = 0;
    int sel_septh = 0;

    // If we don't think we can complete the next depth within the iterative deepening loop before running out of time,
    // we want to stop the search early and save the leftover time. When multiple threads are involved, we don't want
    // the threads stopping early if other threads are continuing. This signal lets the SearchSharedData check which
    // threads want to stop and if all threads do then we stop the search. TODO: could use a consensus model?
    std::atomic<bool> thread_wants_to_stop = false;

    // Set to true when the search is unwinding and trying to return.
    bool aborting_search = false;

    // If set, these restrict the possible root moves considered. A root move will be skipped if it is present in the
    // blacklist, or if it is missing from the whitelist (unless whitelist is empty)
    BasicMoveList root_move_whitelist;
    BasicMoveList root_move_blacklist;

    bool RootExcludeMove(Move move);

    void ResetNewSearch();
    void ResetNewGame();

    //--------------------------------------------------------------------------------------------
private:
    uint64_t padding2[8] = {}; // To avoid false sharing between adjacent SearchData objects
    //--------------------------------------------------------------------------------------------
};

// Search state that is shared between threads.
class SearchSharedState
{
public:
    SearchSharedState(int threads);

    // Below functions are not thread-safe and should not be called during search
    // ------------------------------------

    void ResetNewSearch();
    void ResetNewGame();

    // Below functions are thread-safe and blocking
    // ------------------------------------

    void report_search_result(
        GameState& position, SearchStackState* ss, SearchLocalState& local, int depth, SearchResult result);
    void report_aspiration_low_result(
        GameState& position, SearchStackState* ss, SearchLocalState& local, int depth, SearchResult result);
    void report_aspiration_high_result(
        GameState& position, SearchStackState* ss, SearchLocalState& local, int depth, SearchResult result);

    BasicMoveList get_multi_pv_excluded_moves();

    Move get_best_move() const;
    Score get_best_score() const;

    // Below functions are thread-safe and non-blocking
    // ------------------------------------

    // When one thread completes a particular depth, the other threads can use this to abort early and join at the
    // higher depth
    bool has_completed_depth(int depth) const;

    void report_thread_wants_to_stop(int thread_id);

    int get_next_search_depth() const;

    uint64_t tb_hits() const;
    uint64_t nodes() const;

    SearchLocalState& get_local_state(unsigned int thread_id);

    int get_thread_count() const;

    int multi_pv = 0;
    bool chess_960 = false;

    SearchLimits limits;

private:
    mutable std::mutex lock_;

    enum class SearchInfoType
    {
        EXACT,
        LOWER_BOUND,
        UPPER_BOUND,
    };

    void PrintSearchInfo(GameState& position, SearchStackState* ss, const SearchLocalState& local, int depth,
        Score score, SearchInfoType type) const;

    // For a particular depth, we store the results here. This lets us track how e.g the best move has changed while
    // deepening
    struct SearchDepthResults
    {
        Move best_move = Move::Uninitialized;
        Score score = 0;

        // When reporting lower/upper bound results we want to ensure with multiple threads we only report the most
        // extreme lower/upper bound results.
        Score lowest_alpha = 0;
        Score highest_beta = 0;
    };

    std::array<SearchDepthResults, MAX_DEPTH + 1> search_results_ = {};

    // The depth that has been completed. When the first thread finishes a depth it increments this. All other threads
    // should stop searching that depth
    std::atomic<int> highest_completed_depth_ = 0;

    // We persist the SearchLocalStates for each thread we have, so that they don't need to be reconstructed each time
    // we start a search. search_local_states_.size() == number of threads. This vector is constructed once when the
    // SearchSharedState is created. If the number of threads changes, we need to make a new SearchSharedState.
    std::vector<SearchLocalState> search_local_states_;

    // Moves that we ignore at the root for MultiPV mode
    BasicMoveList multi_PV_excluded_moves_;

    // ----------------------------
    // bool MultiPVExcludeMoveUnlocked(Move move) const;
};
