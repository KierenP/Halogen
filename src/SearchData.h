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
#include "TimeManager.h"
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
class Uci;

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
    const SearchStackState* root() const;
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
    SearchLocalState(int thread_id);

    bool RootExcludeMove(Move move);
    void ResetNewSearch();
    void ResetNewGame();

    const int thread_id;
    SearchStack search_stack;
    EvalCacheTable eval_cache;
    History history;
    int sel_septh = 0;
    std::atomic<uint64_t> tb_hits = 0;
    std::atomic<uint64_t> nodes = 0;

    // track the current depth + multi-pv of the search
    int curr_depth = 0;
    int curr_multi_pv = 0;

    // Set to true when the search is unwinding and trying to return.
    bool aborting_search = false;

    // If we don't think we can complete the next depth within the iterative deepening loop before running out of time,
    // we want to stop the search early and save the leftover time. When multiple threads are involved, we don't want
    // the threads stopping early if other threads are continuing. This signal lets the SearchSharedData check which
    // threads want to stop and if all threads do then we stop the search. TODO: could use a consensus model?
    std::atomic<bool> thread_wants_to_stop = false;

    // If set, these restrict the possible root moves considered. A root move will be skipped if it is present in the
    // blacklist, or if it is missing from the whitelist (unless whitelist is empty)
    BasicMoveList root_move_whitelist;
    BasicMoveList root_move_blacklist;

    // Each time we check the time remaining, we reset this counter to schedule a later time to recheck
    int limit_check_counter = 0;
};

struct SearchResults
{
    int depth = 0;
    int sel_septh = 0;
    int multi_pv = 0;
    Move best_move = Move::Uninitialized;
    Score score = SCORE_UNDEFINED;
    BasicMoveList pv = {};
    SearchResultType type = SearchResultType::EMPTY;
};

// Search state that is shared between threads.
class SearchSharedState
{
public:
    SearchSharedState(int threads, Uci& uci);

    // Below functions are not thread-safe and should not be called during search
    // ------------------------------------

    void ResetNewSearch();
    void ResetNewGame();
    void set_multi_pv(int multi_pv);
    void set_threads(int threads);

    // Below functions are thread-safe and blocking
    // ------------------------------------

    SearchResults get_best_search_result() const;

    void report_search_result(
        const SearchStackState* ss, const SearchLocalState& local, SearchResult result, SearchResultType type);

    // Below functions are thread-safe and non-blocking
    // ------------------------------------

    uint64_t tb_hits() const;
    uint64_t nodes() const;
    int get_threads_setting() const;
    int get_multi_pv_setting() const;

    SearchLocalState& get_local_state(int thread_id);
    void report_thread_wants_to_stop(int thread_id);

    bool chess_960 = false;
    SearchLimits limits;
    Timer search_timer;
    Uci& uci_handler;

private:
    mutable std::recursive_mutex lock_;
    int multi_pv_setting = 1;
    int threads_setting = 0;

    // [thread_id][multi_pv][depth]
    std::vector<std::vector<std::array<SearchResults, MAX_DEPTH + 1>>> search_results_;

    SearchResults* best_search_result_ = nullptr;

    // We persist the SearchLocalStates for each thread we have, so that they don't need to be reconstructed each time
    // we start a search.
    std::vector<std::unique_ptr<SearchLocalState>> search_local_states_;
};
