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
    sel_depth = 0;
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
}

void SearchSharedState::reset_new_game()
{
    transposition_table.clear(get_threads_setting());
    reset_new_search();
}

void SearchSharedState::set_multi_pv(int multi_pv)
{
    multi_pv_setting = multi_pv;
}

void SearchSharedState::set_threads(int threads)
{
    threads_setting = threads;
}

void SearchSharedState::set_hash(int hash_size_mb)
{
    transposition_table.set_size(hash_size_mb, get_threads_setting());
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

SearchInfoData SearchSharedState::build_search_info(int depth, int sel_depth, Score score, int multi_pv,
    const StaticVector<Move, MAX_RECURSION>& pv, SearchResultType type) const
{
    SearchInfoData info;
    info.depth = depth;
    info.sel_depth = sel_depth;
    info.score = score;
    info.time = std::chrono::duration_cast<std::chrono::milliseconds>(search_timer.elapsed());
    info.nodes = nodes();
    info.hashfull = transposition_table.get_hashfull(search_local_states_[0]->position.board().half_turn_count);
    info.tb_hits = tb_hits();
    info.multi_pv = multi_pv;
    info.pv = pv;
    info.type = type;
    return info;
}

SearchInfoData SearchSharedState::get_best_root_move()
{
    // Gather first-root moves from all threads
    std::vector<const RootMove*> cands;
    cands.reserve(search_local_states_.size());
    for (const auto& local : search_local_states_)
    {
        cands.push_back(&local->root_moves[0]);
    }

    // 1) If any thread reports a winning score, accept the shortest win (highest score).
    const RootMove* best_win = nullptr;
    for (const RootMove* r : cands)
    {
        if (!r->score.is_win())
            continue;

        if (!best_win || r->score.value() > best_win->score.value())
        {
            best_win = r;
        }
    }
    if (best_win)
    {
        return build_search_info(
            best_win->search_depth, best_win->sel_depth, best_win->score, 1, best_win->pv, best_win->type);
    }

    // 2) If any thread reports a losing score, take the shortest loss (most negative score).
    const RootMove* best_loss = nullptr;
    for (const RootMove* r : cands)
    {
        if (!r->score.is_loss())
            continue;

        if (!best_loss || r->score.value() < best_loss->score.value())
        {
            best_loss = r;
        }
    }
    if (best_loss)
    {
        return build_search_info(
            best_loss->search_depth, best_loss->sel_depth, best_loss->score, 1, best_loss->pv, best_loss->type);
    }

    // 3) No decisive results: weighted popular vote (weights: depth + scaled score).
    // make score contributions all non-negative by subtracting the lowest thread score
    int min_score = std::numeric_limits<int>::max();
    for (const RootMove* r : cands)
    {
        min_score = std::min(min_score, r->score.value());
    }

    std::unordered_map<Move, float> weight_sum;
    weight_sum.reserve(cands.size());
    for (const RootMove* r : cands)
    {
        float depth = static_cast<float>(r->search_depth);
        float score_diff = static_cast<float>(r->score.value() - min_score);
        weight_sum[r->move] += (depth + 1.0) * (score_diff + 20.0) + 180.0;
    }

    Move chosen_move {};
    float best_weight = -std::numeric_limits<float>::infinity();
    for (const auto& [m, w] : weight_sum)
    {
        if (w > best_weight)
        {
            best_weight = w;
            chosen_move = m;
        }
    }

    // pick the thread that played that move with best depth/score as tie-breaker
    const RootMove* best_root_move = nullptr;
    for (const RootMove* r : cands)
    {
        if (r->move != chosen_move)
            continue;

        if (!best_root_move || r->search_depth > best_root_move->search_depth
            || (r->search_depth == best_root_move->search_depth && r->score > best_root_move->score))
        {
            best_root_move = r;
        }
    }

    return build_search_info(best_root_move->search_depth, best_root_move->sel_depth, best_root_move->score, 1,
        best_root_move->pv, best_root_move->type);
}
