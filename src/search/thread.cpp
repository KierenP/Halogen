#include "thread.h"
#include "bitboard/enum.h"
#include "chessboard/game_state.h"
#include "movegen/list.h"
#include "movegen/movegen.h"
#include "numa/numa.h"
#include "search/data.h"
#include "search/syzygy.h"
#include "uci/uci.h"
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

SearchThread::SearchThread(int thread_id, SearchSharedState& shared_state_)
    : thread_id_(thread_id)
    , shared_state(shared_state_)
    , local_state(make_unique_huge_page<SearchLocalState>(thread_id))
{
}

void SearchThread::thread_loop()
{
    while (!stop)
    {
        std::function<void()> task;

        {
            std::unique_lock lock(mutex);
            cv.wait(lock, [this]() { return !tasks.empty(); });

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();
    }
}

void SearchThread::enqueue_task(std::function<void()> func)
{
    {
        std::lock_guard lock(mutex);
        tasks.push(std::move(func));
    }

    cv.notify_one();
}

void SearchThread::terminate()
{
    // due to the race of signaling the condition variable and destroying it we don't use enqueue_task here
    std::lock_guard lock(mutex);
    tasks.emplace([this]() { stop = true; });
    cv.notify_one();
}

void SearchThread::set_position(std::latch& latch, const GameState& position)
{
    enqueue_task(
        [this, position, &latch]()
        {
            local_state->position = position;
            latch.count_down();
        });
}

void SearchThread::reset_new_search(std::latch& latch)
{
    enqueue_task(
        [this, &latch]()
        {
            local_state->reset_new_search();
            latch.count_down();
        });
}

void SearchThread::reset_new_game(std::latch& latch)
{
    enqueue_task(
        [this, &latch]()
        {
            local_state = std::make_unique<SearchLocalState>(thread_id_);
            latch.count_down();
        });
}

void SearchThread::start_searching(std::latch& latch, const BasicMoveList& root_move_whitelist)
{
    enqueue_task(
        [this, root_move_whitelist, &latch]()
        {
            local_state->root_move_whitelist = root_move_whitelist;
            launch_worker_search(local_state->position, *local_state, shared_state);
            latch.count_down();
        });
}

void SearchThread::update_previous_search_score(std::latch& latch, Score previous_search_score)
{
    enqueue_task(
        [this, previous_search_score, &latch]()
        {
            local_state->prev_search_score = previous_search_score;
            latch.count_down();
        });
}

const SearchLocalState& SearchThread::get_local_state()
{
    return *local_state;
}

SearchThreadPool::SearchThreadPool(UCI::UciOutput& uci, size_t num_threads, size_t multi_pv, size_t hash_size_mb)
    : shared_state(uci)
{
    set_threads(num_threads);
    set_multi_pv(multi_pv);
    set_hash(hash_size_mb);
}

SearchThreadPool::~SearchThreadPool()
{
    for (auto& thread : search_threads)
    {
        thread->terminate();
    }

    for (auto& thread : native_threads)
    {
        thread.join();
    }
}

void SearchThreadPool::set_position(const GameState& position)
{
    position_ = position;
    std::latch latch(search_threads.size());
    for (auto* thread : search_threads)
    {
        thread->set_position(latch, position);
    }
    latch.wait();
}

void SearchThreadPool::reset_new_search()
{
    shared_state.reset_new_search();
    std::latch latch(search_threads.size());
    for (auto* thread : search_threads)
    {
        thread->reset_new_search(latch);
    }
    latch.wait();
}

void SearchThreadPool::reset_new_game()
{
    shared_state.reset_new_game();
    position_ = GameState::starting_position();
    std::latch latch(search_threads.size());
    for (auto* thread : search_threads)
    {
        thread->reset_new_game(latch);
    }
    latch.wait();
}

void SearchThreadPool::set_hash(int hash_size_mb)
{
    shared_state.set_hash(hash_size_mb);
}

void SearchThreadPool::set_multi_pv(int multi_pv)
{
    shared_state.set_multi_pv(multi_pv);
}

void SearchThreadPool::set_chess960(bool chess960)
{
    shared_state.chess_960 = chess960;
}

void SearchThreadPool::set_threads(size_t threads)
{
    shared_state.set_threads(threads);
    if (threads < search_threads.size())
    {
        for (size_t i = threads; i < search_threads.size(); ++i)
        {
            search_threads[i]->terminate();
        }

        for (size_t i = threads; i < native_threads.size(); ++i)
        {
            native_threads[i].join();
        }

        search_threads.resize(threads);
        native_threads.resize(threads);
    }
    else if (threads > search_threads.size())
    {
        for (size_t i = search_threads.size(); i < threads; ++i)
        {
            create_thread();
        }
    }
}

void SearchThreadPool::set_previous_search_score(Score previous_search_score)
{
    std::latch latch(search_threads.size());
    for (auto* thread : search_threads)
    {
        thread->update_previous_search_score(latch, previous_search_score);
    }
    latch.wait();
}

const SearchSharedState& SearchThreadPool::get_shared_state()
{
    return shared_state;
}

void SearchThreadPool::create_thread()
{
    std::promise<SearchThread*> promise;
    auto future = promise.get_future();

    std::thread native_thread(
        [this, &promise]() mutable
        {
            bind_thread(search_threads.size());
            auto thread = std::make_unique<SearchThread>(search_threads.size(), shared_state);
            promise.set_value(thread.get());
            thread->thread_loop();
        });

    native_threads.push_back(std::move(native_thread));
    search_threads.push_back(future.get());
}

void SearchThreadPool::stop_search()
{
    shared_state.stop_searching = true;
}

SearchInfoData SearchThreadPool::launch_search(const SearchLimits& limits)
{
#ifdef TUNE
    LMR_reduction = Initialise_LMR_reduction();
#endif

    reset_new_search();
    shared_state.limits = limits;

    // TODO: a bit ugly
    shared_state.search_local_states_.clear();
    for (auto* thread : search_threads)
    {
        shared_state.search_local_states_.push_back(&thread->get_local_state());
    }

    // Limit the MultiPV setting to be at most the number of legal moves
    auto multi_pv = shared_state.get_multi_pv_setting();
    BasicMoveList moves;
    legal_moves(position_.board(), moves);
    multi_pv = std::min<int>(multi_pv, moves.size());

    // Probe TB at root
    auto probe = Syzygy::probe_dtz_root(position_);
    BasicMoveList root_move_whitelist;
    if (probe.has_value())
    {
        // filter out the results which preserve the tbRank
        for (const auto& [move, tb_rank] : probe->root_moves)
        {
            if (tb_rank != probe->root_moves[0].tb_rank)
            {
                break;
            }

            root_move_whitelist.emplace_back(move);
        }

        multi_pv = std::min<int>(multi_pv, root_move_whitelist.size());
    }

    // TODO: this isn't great. We are resizing the thread results vector for no reason
    auto old_multi_pv = shared_state.get_multi_pv_setting();
    shared_state.set_multi_pv(multi_pv);
    shared_state.stop_searching = false;

    std::latch latch(search_threads.size());
    for (auto* thread : search_threads)
    {
        thread->start_searching(latch, root_move_whitelist);
    }
    latch.wait();

    const auto search_result = shared_state.get_best_root_move();
    shared_state.uci_handler.print_search_info(search_result, true, shared_state.chess_960);
    shared_state.uci_handler.print_bestmove(shared_state.chess_960, search_result.pv[0]);
    shared_state.set_multi_pv(old_multi_pv);
    set_previous_search_score(search_result.score);
    return search_result;
}