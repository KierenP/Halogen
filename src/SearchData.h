#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include <utility>
#include <vector>

#include "BitBoardDefine.h"
#include "EvalCache.h"
#include "History.h"
#include "Move.h"
#include "MoveList.h"
#include "Score.h"
#include "Search.h"
#include "SearchLimits.h"
#include "TranspositionTable.h"

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

extern TranspositionTable tTable;

class GameState;

// Holds information about the search state for a particular recursion depth.
struct SearchStackState
{
    SearchStackState(int distance_from_root_);
    void reset();

    BasicMoveList pv = {};
    std::array<Move, 2> killers = {};

    Move move = Move::Uninitialized;
    Move singular_exclusion = Move::Uninitialized;
    int multiple_extensions = 0;
    const int distance_from_root;
};

class SearchStack
{
    // The search accesses [ss-1, ss+1]
    constexpr static int min_access = -1;
    constexpr static int max_access = 1;
    constexpr static size_t size = MAX_DEPTH + max_access - min_access;

public:
    SearchStackState* root();
    void reset();

private:
    std::array<SearchStackState, size> search_stack_array_ { generate(std::make_integer_sequence<int, size>()) };

    template <int... distances_from_root>
    decltype(search_stack_array_) generate(std::integer_sequence<int, distances_from_root...>)
    {
        return { (distances_from_root + min_access)... };
    }
};

// Data local to a particular thread
struct alignas(hardware_destructive_interference_size) SearchLocalState
{
public:
    SearchStack search_stack;

    EvalCacheTable eval_cache;
    History history;

    std::atomic<uint64_t> tb_hits = 0;
    std::atomic<uint64_t> nodes = 0;
    int sel_septh = 0;
    int search_depth = 0;

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
};

// Search state that is shared between threads.
class SearchSharedState
{
    enum class SearchInfoType
    {
        EXACT,
        LOWER_BOUND,
        UPPER_BOUND,
    };

    struct SearchDepthResults
    {
        Move best_move = Move::Uninitialized;
        Score score = SCORE_UNDEFINED;
        BasicMoveList pv;
    };

public:
    SearchSharedState(int threads);

    // Below functions are not thread-safe and should not be called during search
    // ------------------------------------

    void ResetNewSearch();
    void ResetNewGame();

    // Below functions are thread-safe and blocking
    // ------------------------------------

    void report_search_result(int thread_id, GameState& position, SearchStackState* ss, SearchLocalState& local,
        int depth, SearchResult result);
    void report_aspiration_low_result(
        GameState& position, SearchStackState* ss, SearchLocalState& local, int depth, SearchResult result);
    void report_aspiration_high_result(
        GameState& position, SearchStackState* ss, SearchLocalState& local, int depth, SearchResult result);

    BasicMoveList get_multi_pv_excluded_moves();

    const SearchDepthResults& get_best_search_result() const;

    // Below functions are thread-safe and non-blocking
    // ------------------------------------

    void report_thread_wants_to_stop(int thread_id);

    uint64_t tb_hits() const;
    uint64_t nodes() const;

    SearchLocalState& get_local_state(unsigned int thread_id);

    int get_thread_count() const;

    int multi_pv = 0;
    bool chess_960 = false;

    SearchLimits limits;

private:
    mutable std::mutex lock_;

    void PrintInfoDepthString(GameState& position, const SearchLocalState& local, int depth);

    void PrintSearchInfo(GameState& position, const SearchLocalState& local, int depth, const SearchDepthResults& data,
        SearchInfoType type) const;

    // TODO: fix MultiPV
    std::vector<std::array<SearchDepthResults, MAX_DEPTH + 1>> search_results_;

    // Based on the voting model, this is the current best result
    SearchDepthResults* best_result_ = nullptr;

    // The depth that has been completed. When the first thread finishes a depth it increments this. All other threads
    // should stop searching that depth
    std::atomic<int> highest_completed_depth_ = 0;

    // We persist the SearchLocalStates for each thread we have, so that they don't need to be reconstructed each time
    // we start a search. search_local_states_.size() == number of threads. This vector is constructed once when the
    // SearchSharedState is created. If the number of threads changes, we need to make a new SearchSharedState.
    std::vector<SearchLocalState> search_local_states_;

    // Moves that we ignore at the root for MultiPV mode
    BasicMoveList multi_PV_excluded_moves_;
};
