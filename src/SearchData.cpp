#include "SearchData.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <numeric>

#include "BitBoardDefine.h"
#include "Move.h"
#include "MoveList.h"
#include "Network.h"
#include "Score.h"
#include "Search.h"
#include "uci/uci.h"

TranspositionTable tTable;

SearchStackState::SearchStackState(int distance_from_root_)
    : distance_from_root(distance_from_root_)
{
}

void SearchStackState::reset()
{
    pv = {};
    killers = {};
    move = Move::Uninitialized;
    singular_exclusion = Move::Uninitialized;
    multiple_extensions = 0;
    acc[BLACK].Reset();
    acc[WHITE].Reset();
}

const SearchStackState* SearchStack::root() const
{
    return &search_stack_array_[-min_access];
}

SearchStackState* SearchStack::root()
{
    return &search_stack_array_[-min_access];
}

void SearchStack::reset()
{
    for (auto& sss : search_stack_array_)
    {
        sss.reset();
    }
}

SearchLocalState::SearchLocalState(int thread_id_)
    : thread_id(thread_id_)
{
}

bool SearchLocalState::RootExcludeMove(Move move)
{
    // if present in blacklist, exclude
    if (std::find(root_move_blacklist.begin(), root_move_blacklist.end(), move) != root_move_blacklist.end())
    {
        return true;
    }

    // if not present in non-empty white list
    if (!root_move_whitelist.empty()
        && std::find(root_move_whitelist.begin(), root_move_whitelist.end(), move) == root_move_whitelist.end())
    {
        return true;
    }

    return false;
}

void SearchLocalState::ResetNewSearch()
{
    // We don't reset the history tables because it gains elo to perserve them between turns
    search_stack.reset();
    tb_hits = 0;
    nodes = 0;
    sel_septh = 0;
    curr_depth = 0;
    curr_multi_pv = 0;
    thread_wants_to_stop = false;
    aborting_search = false;
    root_move_blacklist = {};
    root_move_whitelist = {};
    limit_check_counter = 0;
}

void SearchLocalState::ResetNewGame()
{
    ResetNewSearch();
    history.reset();
}

SearchSharedState::SearchSharedState(Uci& uci)
    : uci_handler(uci)
{
}

void SearchSharedState::ResetNewSearch()
{
    for (auto& thread_results : search_results_)
    {
        for (auto& multi_pv_results : thread_results)
        {
            multi_pv_results = {};
        }
    }

    best_search_result_ = nullptr;
    search_timer.reset();
    std::for_each(search_local_states_.begin(), search_local_states_.end(), [](auto& data) { data->ResetNewSearch(); });
}

void SearchSharedState::ResetNewGame()
{
    ResetNewSearch();
    std::for_each(search_local_states_.begin(), search_local_states_.end(), [](auto& data) { data->ResetNewGame(); });
}

void SearchSharedState::set_multi_pv(int multi_pv)
{
    multi_pv_setting = multi_pv;

    for (auto& thread_results : search_results_)
    {
        thread_results.resize(multi_pv);
    }
}

void SearchSharedState::set_threads(int threads)
{
    threads_setting = threads;

    search_local_states_.clear();
    for (int i = 0; i < threads_setting; i++)
    {
        search_local_states_.emplace_back(std::make_unique<SearchLocalState>(i));
    }

    search_results_.resize(threads, decltype(search_results_)::value_type(multi_pv_setting));
}

SearchResults SearchSharedState::get_best_search_result() const
{
    std::scoped_lock lock(lock_);
    return *best_search_result_;
}

void SearchSharedState::report_search_result(
    const SearchStackState* ss, const SearchLocalState& local, SearchResult result, SearchResultType type)
{
    std::scoped_lock lock(lock_);

    // Store the result in the table (potentially overwriting a previous lower/upper bound)
    auto& result_data = search_results_[local.thread_id][local.curr_multi_pv - 1][local.curr_depth];
    result_data
        = { local.curr_depth, local.sel_septh, local.curr_multi_pv, result.GetMove(), result.GetScore(), ss->pv, type };

    // Update the best search result. We want to pick the highest depth result, and using the higher score for
    // tie-breaks. It adds elo to also include LOWER_BOUND search results as potential best result candidates.
    if ((result_data.type == SearchResultType::EXACT || result_data.type == SearchResultType::LOWER_BOUND)
        && (!best_search_result_ || (best_search_result_->depth < result_data.depth)
            || (best_search_result_->depth == result_data.depth && best_search_result_->score < result_data.score)))
    {
        best_search_result_ = &result_data;
    }

    // Only the main thread prints info output. We limit lowerbound/upperbound info results to after the first 5 seconds
    // of search to avoid printing too much output
    if (local.thread_id == 0)
    {
        using namespace std::chrono_literals;
        if (type == SearchResultType::EXACT || search_timer.elapsed() > 5000ms)
        {
            uci_handler.print_search_info(result_data);
        }
    }
}

uint64_t SearchSharedState::tb_hits() const
{
    return std::accumulate(search_local_states_.begin(), search_local_states_.end(), (uint64_t)0,
        [](const auto& val, const auto& state) { return val + state->tb_hits; });
}

uint64_t SearchSharedState::nodes() const
{
    return std::accumulate(search_local_states_.begin(), search_local_states_.end(), (uint64_t)0,
        [](const auto& val, const auto& state) { return val + state->nodes; });
}

int SearchSharedState::get_threads_setting() const
{
    return threads_setting;
}

int SearchSharedState::get_multi_pv_setting() const
{
    return multi_pv_setting;
}

SearchLocalState& SearchSharedState::get_local_state(int thread_id)
{
    return *search_local_states_[thread_id];
}

void SearchSharedState::report_thread_wants_to_stop(int thread_id)
{
    // If at least half the threads (rounded up) want to stop, we abort

    search_local_states_[thread_id]->thread_wants_to_stop = true;

    int abort_votes = 0;

    for (const auto& local : search_local_states_)
    {
        if (local->thread_wants_to_stop == true)
        {
            abort_votes++;
        }
    }

    if (abort_votes * 2 >= threads_setting)
    {
        stop_searching = true;
    }
}
