#include "search/data.h"

#include <algorithm>
#include <chrono>
#include <compare>
#include <cstdint>
#include <mutex>
#include <numeric>
#include <string>

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "search/score.h"
#include "search/transposition/table.h"
#include "uci/uci.h"

SearchStackState::SearchStackState(int distance_from_root_)
    : distance_from_root(distance_from_root_)
{
}

const SearchStackState* SearchStack::root() const
{
    return &search_stack_array_[-min_access];
}

SearchStackState* SearchStack::root()
{
    return &search_stack_array_[-min_access];
}

const NN::Accumulator* AccumulatorStack::root() const
{
    return &acc_stack_[0];
}

NN::Accumulator* AccumulatorStack::root()
{
    return &acc_stack_[0];
}

SearchLocalState::SearchLocalState(int thread_id_)
    : thread_id(thread_id_)
{
}

bool SearchLocalState::should_skip_root_move(Move move)
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

void SearchLocalState::reset_new_search()
{
    // We don't reset the history tables because it gains elo to perserve them between turns
    search_stack = {};
    acc_stack = {};
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
    root_moves = {};

    net.reset_new_search(position.board(), *acc_stack.root());
    BasicMoveList moves;
    legal_moves(position.board(), moves);
    std::ranges::copy(moves, std::back_inserter(root_moves));
}

int SearchLocalState::get_quiet_search_history(const SearchStackState* ss, Move move)
{
    int total = 0;

    if (auto* hist = pawn_hist.get(position.board(), ss, move))
    {
        total += *hist;
    }

    if (auto* hist = threat_hist.get(position.board(), ss, move))
    {
        total += *hist;
    }

    if ((ss - 1)->cont_hist_subtable)
    {
        total += *(ss - 1)->cont_hist_subtable->get(position.board(), ss, move);
    }

    if ((ss - 2)->cont_hist_subtable)
    {
        total += *(ss - 2)->cont_hist_subtable->get(position.board(), ss, move);
    }

    return total;
}

int SearchLocalState::get_quiet_order_history(const SearchStackState* ss, Move move)
{
    int total = 0;

    if (auto* hist = pawn_hist.get(position.board(), ss, move))
    {
        total += *hist;
    }

    if (auto* hist = threat_hist.get(position.board(), ss, move))
    {
        total += *hist;
    }

    if ((ss - 1)->cont_hist_subtable)
    {
        total += *(ss - 1)->cont_hist_subtable->get(position.board(), ss, move);
    }

    if ((ss - 2)->cont_hist_subtable)
    {
        total += *(ss - 2)->cont_hist_subtable->get(position.board(), ss, move);
    }

    if ((ss - 4)->cont_hist_subtable)
    {
        total += *(ss - 4)->cont_hist_subtable->get(position.board(), ss, move);
    }

    return total;
}

int SearchLocalState::get_loud_history(const SearchStackState* ss, Move move)
{
    int total = 0;
    if (auto* hist = capt_hist.get(position.board(), ss, move))
    {
        total += *hist;
    }
    return total;
}

void SearchLocalState::add_quiet_history(const SearchStackState* ss, Move move, int change)
{
    pawn_hist.add(position.board(), ss, move, change);
    threat_hist.add(position.board(), ss, move, change);
    if ((ss - 1)->cont_hist_subtable)
        (ss - 1)->cont_hist_subtable->add(position.board(), ss, move, change);
    if ((ss - 2)->cont_hist_subtable)
        (ss - 2)->cont_hist_subtable->add(position.board(), ss, move, change);
    if ((ss - 4)->cont_hist_subtable)
        (ss - 4)->cont_hist_subtable->add(position.board(), ss, move, change);
}

void SearchLocalState::add_loud_history(const SearchStackState* ss, Move move, int change)
{
    capt_hist.add(position.board(), ss, move, change);
}

SearchSharedState::SearchSharedState(UCI::UciOutput& uci)
    : uci_handler(uci)
{
}

void SearchSharedState::reset_new_search()
{
    search_timer.reset();

    for (auto& thread_results : search_results_)
    {
        for (auto& multi_pv_results : thread_results)
        {
            multi_pv_results = {};
        }
    }

    best_search_result_ = {};
}

void SearchSharedState::reset_new_game()
{
    transposition_table.clear(get_threads_setting());
    reset_new_search();
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
    search_results_.resize(threads, decltype(search_results_)::value_type(multi_pv_setting));
}

void SearchSharedState::set_hash(int hash_size_mb)
{
    transposition_table.set_size(hash_size_mb, get_threads_setting());
}

SearchResults SearchSharedState::get_best_search_result() const
{
    std::scoped_lock lock(lock_);
    if (!best_search_result_)
    {
        uci_handler.print_error("Could not find best move");
    }
    return best_search_result_.value_or(SearchResults {});
}

void SearchSharedState::set_best_search_result(const SearchResults& result)
{
    std::scoped_lock lock(lock_);
    best_search_result_ = result;
}

void SearchSharedState::report_search_result(
    const SearchStackState* ss, const SearchLocalState& local, Score score, SearchResultType type)
{
    std::scoped_lock lock(lock_);

    // Only the main thread prints info output. We limit lowerbound/upperbound info results to after the first 5 seconds
    // of search to avoid printing too much output
    if (local.thread_id == 0)
    {
        using namespace std::chrono_literals;
        if (type == SearchResultType::EXACT || search_timer.elapsed() > 5000ms)
        {
            uci_handler.print_search_info(*this,
                { local.curr_depth, local.sel_septh, local.curr_multi_pv, nodes(), score, ss->pv, type },
                local.position.board());
        }
    }
}

int64_t SearchSharedState::tb_hits() const
{
    return std::accumulate(search_local_states_.begin(), search_local_states_.end(), (int64_t)0,
        [](const auto& val, const auto& state) { return val + state->tb_hits; });
}

int64_t SearchSharedState::nodes() const
{
    return std::accumulate(search_local_states_.begin(), search_local_states_.end(), (int64_t)0,
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

void SearchSharedState::report_thread_wants_to_stop()
{
    // If at least half the threads (rounded up) want to stop, we abort
    int abort_votes = 0;

    for (const auto& local : search_local_states_)
    {
        if (local->thread_wants_to_stop == true)
        {
            abort_votes++;
        }
    }

    // TODO: SPSA SMP tune this?
    if (abort_votes * 2 >= threads_setting)
    {
        stop_searching = true;
    }
}
