#include "SearchData.h"

#include <atomic>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdlib.h>

#include "BitBoardDefine.h"
#include "GameState.h"
#include "Move.h"
#include "MoveList.h"
#include "Score.h"
#include "Search.h"
#include "Zobrist.h"

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
    search_depth = 0;
    thread_wants_to_stop = false;
    aborting_search = false;
    root_move_blacklist = {};
    root_move_whitelist = {};
}

void SearchLocalState::ResetNewGame()
{
    ResetNewSearch();
    history.reset();
}

SearchSharedState::SearchSharedState(int threads)
    : search_results_(threads)
    , search_local_states_(threads)
{
}

void SearchSharedState::report_search_result(
    int thread_id, GameState& position, SearchStackState* ss, SearchLocalState& local, int depth, SearchResult result)
{
    std::scoped_lock lock(lock_);

    auto& results = search_results_[thread_id][depth];
    results.best_move = result.GetMove();
    results.score = result.GetScore();
    results.pv = ss->pv;

    PrintInfoDepthString(position, local, depth);
}

void SearchSharedState::report_aspiration_low_result(
    GameState&, SearchStackState*, SearchLocalState&, int, SearchResult)
{
    /*std::scoped_lock lock(lock_);

    auto elapsed_time = limits.ElapsedTime();
    auto completed_depth = highest_completed_depth_.load(std::memory_order_acquire);

    if (depth == completed_depth + 1 && result.GetScore() < search_results_[depth].lowest_alpha)
    {
        if (elapsed_time > 5000)
        {
            PrintSearchInfo(position, ss, local, depth, result.GetScore(), SearchInfoType::UPPER_BOUND);
        }
        search_results_[depth].lowest_alpha = result.GetScore();
    }*/
}

void SearchSharedState::report_aspiration_high_result(
    GameState&, SearchStackState*, SearchLocalState&, int, SearchResult)
{
    /*std::scoped_lock lock(lock_);

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
    }*/
}

void SearchSharedState::report_thread_wants_to_stop(int thread_id)
{
    // If at least half the threads (rounded up) want to stop, we abort

    search_local_states_[thread_id].thread_wants_to_stop = true;

    int abort_votes = 0;

    for (const auto& local : search_local_states_)
    {
        if (local.thread_wants_to_stop == true)
        {
            abort_votes++;
        }
    }

    if (abort_votes * 2 >= (int)search_local_states_.size())
    {
        KeepSearching = false;
    }
}

BasicMoveList SearchSharedState::get_multi_pv_excluded_moves()
{
    std::scoped_lock lock(lock_);
    return multi_PV_excluded_moves_;
}

const SearchSharedState::SearchDepthResults& SearchSharedState::get_best_search_result() const
{
    std::scoped_lock lock(lock_);

    // of each thread, we want the highest depth result. If multiple threads have reached the same depth, we pick the
    // result with the highest score

    const SearchDepthResults* best = nullptr;

    for (int depth = MAX_DEPTH; depth >= 0; depth--)
    {
        for (int thread = 0; thread < get_thread_count(); thread++)
        {
            const auto& result = search_results_[thread][depth];
            if (result.score == SCORE_UNDEFINED)
            {
                continue;
            }

            if (!best || best->score < result.score)
            {
                best = &result;
            }
        }

        if (best)
        {
            return *best;
        }
    }

    assert(false);
    return search_results_[0][0];
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
    search_results_.clear();
    search_results_.resize(get_thread_count());
    best_result_ = nullptr;
    highest_completed_depth_ = 0;
    multi_PV_excluded_moves_.clear();

    std::for_each(search_local_states_.begin(), search_local_states_.end(), [](auto& data) { data.ResetNewSearch(); });
}

void SearchSharedState::ResetNewGame()
{
    search_results_.clear();
    search_results_.resize(get_thread_count());
    highest_completed_depth_ = 0;
    multi_PV_excluded_moves_.clear();

    std::for_each(search_local_states_.begin(), search_local_states_.end(), [](auto& data) { data.ResetNewGame(); });
}

void SearchSharedState::PrintInfoDepthString(GameState& position, const SearchLocalState& local, int depth)
{
    // check if every thread has finished this depth: if so, print out the result with the highest score.
    SearchDepthResults* best = nullptr;

    for (int thread_id = 0; thread_id < get_thread_count(); thread_id++)
    {
        auto& result = search_results_[thread_id][depth];

        if (result.score == SCORE_UNDEFINED)
        {
            return;
        }

        if (!best || best->score < result.score)
        {
            best = &result;
        }
    }

    PrintSearchInfo(position, local, depth, *best, SearchInfoType::EXACT);
}

void SearchSharedState::PrintSearchInfo(GameState& position, const SearchLocalState& local, int depth,
    const SearchDepthResults& data, SearchInfoType type) const
{
    /*
    Here we avoid excessive use of std::cout and instead append to a string in order
    to output only once at the end. This causes a noticeable speedup for very fast
    time controls.
    */

    std::stringstream stream;

    stream << "info depth " << depth << " seldepth " << local.sel_septh;

    if (Score(abs(data.score.value())) > Score::mate_in(MAX_DEPTH))
    {
        if (data.score > 0)
            stream << " score mate " << ((Score::Limits::MATE - abs(data.score.value())) + 1) / 2;
        else
            stream << " score mate " << -((Score::Limits::MATE - abs(data.score.value())) + 1) / 2;
    }
    else
    {
        stream << " score cp " << data.score.value();
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

    for (const auto& move : data.pv)
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

    for (size_t i = 0; i < data.pv.size(); i++)
    {
        position.RevertMove();
    }

    std::cout << stream.str() << std::endl;
}
