#include "thread.h"
#include "chessboard/game_state.h"
#include "movegen/list.h"
#include "movegen/movegen.h"
#include "numa/numa.h"
#include "search/data.h"
#include "search/syzygy.h"
#include "uci/uci.h"
#include <future>
#include <iostream>
#include <thread>

SearchThread::SearchThread(int thread_id, SearchSharedState& shared_state_)
    : thread_id_(thread_id)
    , shared_state(shared_state_)
    , local_state(std::make_unique<SearchLocalState>(thread_id))
{
}

void SearchThread::thread_loop()
{
    while (!stop)
    {
        std::packaged_task<void()> task;

        {
            std::unique_lock lock(mutex);
            cv.wait(lock, [this]() { return !tasks.empty(); });

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();
    }
}

std::future<void> SearchThread::enqueue_task(std::function<void()> func)
{
    std::packaged_task task(std::move(func));
    auto future = task.get_future();

    {
        std::lock_guard lock(mutex);
        tasks.push(std::move(task));
    }

    cv.notify_one();
    return future;
}

void SearchThread::terminate()
{
    // due to the race of signaling the condition variable and destroying it we don't use enqueue_task here
    std::lock_guard lock(mutex);
    tasks.emplace([this]() { stop = true; });
    cv.notify_one();
}

std::future<void> SearchThread::set_position(const GameState& position)
{
    return enqueue_task([this, position]() { local_state->position = position; });
}

std::future<void> SearchThread::reset_new_search()
{
    return enqueue_task([this]() { local_state->reset_new_search(); });
}

std::future<void> SearchThread::reset_new_game()
{
    return enqueue_task([this]() { local_state = std::make_unique<SearchLocalState>(thread_id_); });
}

std::future<void> SearchThread::start_searching(const BasicMoveList& root_move_whitelist)
{
    return enqueue_task(
        [this, root_move_whitelist]()
        {
            local_state->root_move_whitelist = root_move_whitelist;
            launch_worker_search(local_state->position, *local_state, shared_state);
        });
}

const SearchLocalState& SearchThread::get_local_state()
{
    return *local_state;
}

SearchThreadPool::SearchThreadPool(UCI::UciOutput& uci, size_t num_threads)
    : shared_state(uci)
{
    for (size_t i = 0; i < num_threads; ++i)
    {
        create_thread();
    }
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
    std::vector<std::future<void>> futures;
    for (auto* thread : search_threads)
    {
        futures.push_back(thread->set_position(position));
    }
    for (auto& future : futures)
    {
        future.get();
    }
}

void SearchThreadPool::reset_new_search()
{
    shared_state.reset_new_search();
    std::vector<std::future<void>> futures;
    for (auto* thread : search_threads)
    {
        futures.push_back(thread->reset_new_search());
    }
    for (auto& future : futures)
    {
        future.get();
    }
}

void SearchThreadPool::reset_new_game()
{
    position_ = GameState::starting_position();
    std::vector<std::future<void>> futures;
    for (auto* thread : search_threads)
    {
        futures.push_back(thread->reset_new_game());
    }
    for (auto& future : futures)
    {
        future.get();
    }
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

SearchResults SearchThreadPool::launch_search(const SearchLimits& limits)
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
    auto probe = Syzygy::probe_dtz_root(position_.board());
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

    std::vector<std::future<void>> futures;
    futures.reserve(search_threads.size());

    for (auto* thread : search_threads)
    {
        futures.push_back(thread->start_searching(root_move_whitelist));
    }

    for (auto& future : futures)
    {
        future.get();
    }

    const auto& search_result = shared_state.get_best_search_result();
    shared_state.uci_handler.print_search_info(shared_state, search_result, position_.board(), true);
    shared_state.uci_handler.print_bestmove(shared_state.chess_960, search_result.pv[0]);
    shared_state.set_multi_pv(old_multi_pv);
    return search_result;
}