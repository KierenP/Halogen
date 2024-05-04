#include "SearchData.h"

#include <assert.h>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdlib.h>

#include "BitBoardDefine.h"
#include "GameState.h"
#include "MoveList.h"
#include "Search.h"
#include "Zobrist.h"

TranspositionTable tTable;

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
    search_stack = {};
    tb_hits = 0;
    nodes = 0;
    sel_septh = 0;
    thread_wants_to_stop = false;
    aborting_search = false;
    root_move_blacklist = {};
    root_move_whitelist = {};
}

void SearchLocalState::ResetNewGame()
{
    search_stack = {};
    tb_hits = 0;
    nodes = 0;
    sel_septh = 0;
    thread_wants_to_stop = false;
    aborting_search = false;
    history.reset();
}

SearchSharedState::SearchSharedState(int threads)
    : search_local_states_(threads)
{
}

void SearchSharedState::report_search_result(
    GameState& position, SearchStackState* ss, SearchLocalState& local, int depth, SearchResult result)
{
    std::scoped_lock lock(lock_);
    auto completed_depth = highest_completed_depth_.load(std::memory_order_acquire);

    if (depth > completed_depth
        && std::find(multi_PV_excluded_moves_.begin(), multi_PV_excluded_moves_.end(), result.GetMove())
            == multi_PV_excluded_moves_.end())
    {
        // The way we do MultiPV is a little wrong. Each time we complete the search at depth N we exclude that move
        // from being used at the root and repeat the search. Once we have MultiPV searches complete at depth N then we
        // move to depth N+1. The correct approach would be to store all depth N results until we have sufficiently
        // many, and then print them all out ordered by score

        PrintSearchInfo(position, ss, local, depth, result.GetScore(), SearchInfoType::EXACT);

        // If this is the first time we have completed the search at depth N, record it into the table.
        if (multi_PV_excluded_moves_.size() == 0)
        {
            search_results_[depth].best_move = result.GetMove();
            search_results_[depth].score = result.GetScore();
            search_results_[depth + 1].highest_beta = result.GetScore();
            search_results_[depth + 1].lowest_alpha = result.GetScore();
        }

        multi_PV_excluded_moves_.push_back(result.GetMove());

        // Once we have reported results for multi_pv searches, we continue to the next depth
        if (multi_PV_excluded_moves_.size() >= (unsigned)multi_pv)
        {
            highest_completed_depth_.store(depth, std::memory_order_release);
            multi_PV_excluded_moves_.clear();
        }
    }
}

void SearchSharedState::report_aspiration_low_result(
    GameState& position, SearchStackState* ss, SearchLocalState& local, int depth, SearchResult result)
{
    std::scoped_lock lock(lock_);

    auto elapsed_time = limits.ElapsedTime();
    auto completed_depth = highest_completed_depth_.load(std::memory_order_acquire);

    if (depth == completed_depth + 1 && result.GetScore() < search_results_[depth].lowest_alpha)
    {
        if (elapsed_time > 5000)
        {
            PrintSearchInfo(position, ss, local, depth, result.GetScore(), SearchInfoType::UPPER_BOUND);
        }
        search_results_[depth].lowest_alpha = result.GetScore();
    }
}

void SearchSharedState::report_aspiration_high_result(
    GameState& position, SearchStackState* ss, SearchLocalState& local, int depth, SearchResult result)
{
    std::scoped_lock lock(lock_);

    auto elapsed_time = limits.ElapsedTime();
    auto completed_depth = highest_completed_depth_.load(std::memory_order_acquire);

    if (depth == completed_depth + 1 && result.GetScore() > search_results_[depth].highest_beta)
    {
        if (elapsed_time > 5000)
        {
            PrintSearchInfo(position, ss, local, depth, result.GetScore(), SearchInfoType::LOWER_BOUND);
        }
        search_results_[depth].highest_beta = result.GetScore();

        // When we fail high, use set this as the best move for the next depth. This gains elo becuase more often than
        // not a fail high move turns out to be the best.
        search_results_[completed_depth + 1].best_move = result.GetMove();
    }
}

bool SearchSharedState::has_completed_depth(int depth) const
{
    return highest_completed_depth_.load(std::memory_order_acquire) >= depth;
}

void SearchSharedState::report_thread_wants_to_stop(int thread_id)
{
    // If all other threads also want to stop, we mark KeepSearching as false to signal the threads to stop.
    search_local_states_[thread_id].thread_wants_to_stop = true;

    for (const auto& local : search_local_states_)
    {
        if (local.thread_wants_to_stop == false)
            return;
    }

    KeepSearching = false;
}

int SearchSharedState::get_next_search_depth() const
{
    return highest_completed_depth_.load(std::memory_order_acquire) + 1;
}

BasicMoveList SearchSharedState::get_multi_pv_excluded_moves()
{
    std::scoped_lock lock(lock_);
    return multi_PV_excluded_moves_;
}

Move SearchSharedState::get_best_move() const
{
    std::scoped_lock lock(lock_);
    auto completed_depth = highest_completed_depth_.load(std::memory_order_acquire);

    // On a fail high we will report the best move. So we check at depth + 1 and return that if it's been set
    if (search_results_[completed_depth + 1].best_move != Move::Uninitialized)
    {
        return search_results_[completed_depth + 1].best_move;
    }

    return search_results_[completed_depth].best_move;
}

Score SearchSharedState::get_best_score() const
{
    std::scoped_lock lock(lock_);
    auto completed_depth = highest_completed_depth_.load(std::memory_order_acquire);

    return search_results_[completed_depth].score;
}

uint64_t SearchSharedState::tb_hits() const
{
    return std::accumulate(search_local_states_.begin(), search_local_states_.end(), (uint64_t)0,
        [](const auto& val, const auto& state) { return val + state.tb_hits.load(std::memory_order_relaxed); });
}

uint64_t SearchSharedState::nodes() const
{
    return std::accumulate(search_local_states_.begin(), search_local_states_.end(), (uint64_t)0,
        [](const auto& val, const auto& state) { return val + state.nodes.load(std::memory_order_relaxed); });
}

SearchLocalState& SearchSharedState::get_local_state(unsigned int thread_id)
{
    return search_local_states_[thread_id];
}

int SearchSharedState::get_thread_count() const
{
    return search_local_states_.size();
}

void SearchSharedState::ResetNewSearch()
{
    search_results_ = {};
    highest_completed_depth_ = 0;
    multi_PV_excluded_moves_.clear();

    std::for_each(search_local_states_.begin(), search_local_states_.end(), [](auto& data) { data.ResetNewSearch(); });
}

void SearchSharedState::ResetNewGame()
{
    search_results_ = {};
    highest_completed_depth_ = 0;
    multi_PV_excluded_moves_.clear();

    std::for_each(search_local_states_.begin(), search_local_states_.end(), [](auto& data) { data.ResetNewGame(); });
}

void SearchSharedState::PrintSearchInfo(GameState& position, SearchStackState* ss, const SearchLocalState& local,
    int depth, Score score, SearchInfoType type) const
{
    /*
    Here we avoid excessive use of std::cout and instead append to a string in order
    to output only once at the end. This causes a noticeable speedup for very fast
    time controls.
    */

    std::stringstream stream;

    stream << "info depth " << depth << " seldepth " << local.sel_septh;

    if (Score(abs(score.value())) > Score::mate_in(MAX_DEPTH))
    {
        if (score > 0)
            stream << " score mate " << ((Score::Limits::MATE - abs(score.value())) + 1) / 2;
        else
            stream << " score mate " << -((Score::Limits::MATE - abs(score.value())) + 1) / 2;
    }
    else
    {
        stream << " score cp " << score.value();
    }

    if (type == SearchInfoType::UPPER_BOUND)
        stream << " upperbound";
    if (type == SearchInfoType::LOWER_BOUND)
        stream << " lowerbound";

    auto elapsed_time = limits.ElapsedTime();
    auto node_count = nodes();
    auto nps = node_count / std::max(elapsed_time, 1) * 1000;
    auto hashfull = tTable.GetCapacity(position.Board().half_turn_count);

    stream << " time " << elapsed_time << " nodes " << node_count << " nps " << nps << " hashfull " << hashfull
           << " tbhits " << tb_hits();

    if (multi_pv > 0)
        stream << " multipv " << multi_PV_excluded_moves_.size() + 1;

    stream << " pv "; // the current best line found

    for (const auto& move : ss->pv)
    {
        if (chess_960)
        {
            stream << move.to_string_960(position.Board().stm, position.Board().castle_squares);
        }
        else
        {
            stream << move.to_string();
        }

        // for chess960 positions to be printed correctly, we must have the correct board state at time of printing
        position.ApplyMove(move);

        stream << " ";
    }

    for (size_t i = 0; i < ss->pv.size(); i++)
    {
        position.RevertMove();
    }

    std::cout << stream.str() << std::endl;
}
