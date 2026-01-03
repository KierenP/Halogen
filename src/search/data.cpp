#include "search/data.h"

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "search/score.h"
#include "search/transposition/table.h"
#include "spsa/tuneable.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iterator>
#include <numeric>
#include <ranges>
#include <unordered_map>

namespace
{
const SearchStack default_search_stack {};
const AccumulatorStack default_acc_stack {};
}

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

SearchLocalState::SearchLocalState(int thread_id_, SharedHistory* corr_hist_)
    : thread_id(thread_id_)
    , shared_hist(corr_hist_)
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
    search_stack = default_search_stack;
    acc_stack = default_acc_stack;
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

    if (auto* hist = shared_hist->pawn_hist.get(position.board(), ss, move))
    {
        total += *hist;
    }

    if (auto* hist = shared_hist->threat_hist.get(position.board(), ss, move))
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

    if (auto* hist = shared_hist->pawn_hist.get(position.board(), ss, move))
    {
        total += *hist;
    }

    if (auto* hist = shared_hist->threat_hist.get(position.board(), ss, move))
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

void SearchLocalState::add_quiet_history(const SearchStackState* ss, Move move, Fraction<64> change)
{
    shared_hist->pawn_hist.add(position.board(), ss, move, change);
    shared_hist->threat_hist.add(position.board(), ss, move, change);
    if ((ss - 1)->cont_hist_subtable)
        (ss - 1)->cont_hist_subtable->add(position.board(), ss, move, change);
    if ((ss - 2)->cont_hist_subtable)
        (ss - 2)->cont_hist_subtable->add(position.board(), ss, move, change);
    if ((ss - 4)->cont_hist_subtable)
        (ss - 4)->cont_hist_subtable->add(position.board(), ss, move, change);
}

void SearchLocalState::add_loud_history(const SearchStackState* ss, Move move, Fraction<64> change)
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
    shared_hist_ = std::make_unique<PerNumaAllocation<SharedHistory>>();
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
    std::ranges::transform(
        search_local_states_, std::back_inserter(cands), [](const auto& local) { return &local->root_moves[0]; });

    // 1) If any thread reports a winning score, accept the shortest win (highest score).
    {
        auto wins_view = cands | std::views::filter([](const auto* r) { return r->score.is_win(); });
        if (auto it = std::ranges::max_element(wins_view, {}, &RootMove::score); it != std::ranges::end(wins_view))
        {
            const RootMove* best_win = *it;
            return build_search_info(
                best_win->search_depth, best_win->sel_depth, best_win->score, 1, best_win->pv, best_win->type);
        }
    }

    // 2) If any thread reports a losing score, take the shortest loss (lowest scire).
    {
        auto losses_view = cands | std::views::filter([](const auto* r) { return r->score.is_loss(); });
        if (auto it = std::ranges::min_element(losses_view, {}, &RootMove::score); it != std::ranges::end(losses_view))
        {
            const RootMove* best_loss = *it;
            return build_search_info(
                best_loss->search_depth, best_loss->sel_depth, best_loss->score, 1, best_loss->pv, best_loss->type);
        }
    }

    // 3) No decisive results: weighted popular vote (weights: depth + scaled score).
    // make score contributions all non-negative by subtracting the lowest thread score
    const int min_score = (*std::ranges::min_element(cands, {}, &RootMove::score))->score.value();

    std::unordered_map<Move, float> weight_sum;
    for (const RootMove* r : cands)
    {
        const auto depth = static_cast<float>(r->search_depth);
        const auto score_diff = static_cast<float>(r->score.value() - min_score);
        weight_sum[r->move] += (depth + smp_voting_depth) * (score_diff + smp_voting_score) + smp_voting_const;
    }

    // choose move with highest total weight
    const auto chosen_move
        = (*std::ranges::max_element(weight_sum, {}, [](const auto& move_votes) { return move_votes.second; })).first;

    // pick the thread that played that move with best depth/score as tie-breaker
    auto same_move_view = cands | std::views::filter([&](const RootMove* r) { return r->move == chosen_move; });
    const auto* best_root_move = *std::ranges::max_element(same_move_view,
        [](const RootMove* a, const RootMove* b)
        {
            if (a->search_depth != b->search_depth)
                return a->search_depth < b->search_depth;
            return a->score < b->score;
        });

    return build_search_info(best_root_move->search_depth, best_root_move->sel_depth, best_root_move->score, 1,
        best_root_move->pv, best_root_move->type);
}

SharedHistory* SearchSharedState::get_shared_hist(size_t thread_index)
{
    return shared_hist_->get(thread_index);
}