#pragma once

#include "chessboard/game_state.h"
#include "search/data.h"
#include "search/search.h"

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

class SearchThread
{
public:
    SearchThread(int thread_id, SearchSharedState& shared_state);

    void thread_loop();
    void terminate();

    [[nodiscard]] std::future<void> set_position(const GameState& position);
    [[nodiscard]] std::future<void> reset_new_search();
    [[nodiscard]] std::future<void> reset_new_game();
    [[nodiscard]] std::future<void> start_searching(const BasicMoveList& root_move_whitelist);

    const SearchLocalState& get_local_state();

private:
    std::mutex mutex;
    std::condition_variable cv;
    std::queue<std::packaged_task<void()>> tasks;
    bool stop = false;

    std::future<void> enqueue_task(std::function<void()> func);

    const int thread_id_;
    SearchSharedState& shared_state;
    std::unique_ptr<SearchLocalState> local_state;
};

class SearchThreadPool
{
public:
    SearchThreadPool(UCI::UciOutput& uci, size_t num_threads);
    ~SearchThreadPool();

    SearchThreadPool(const SearchThreadPool&) = delete;
    SearchThreadPool& operator=(const SearchThreadPool&) = delete;
    SearchThreadPool(SearchThreadPool&&) = delete;
    SearchThreadPool& operator=(SearchThreadPool&&) = delete;

    void reset_new_search();
    void reset_new_game();

    // TODO: some of this stuff is specific to the thread pool, but some of it is really only here because
    // SearchThreadPool for some reason owns the SearchSharedState
    void set_position(const GameState& position);
    void set_hash(int hash_size_mb);
    void set_multi_pv(int multi_pv);
    void set_chess960(bool chess960)
    {
        shared_state.chess_960 = chess960;
    }
    void set_threads(size_t threads);

    SearchResults launch_search(const SearchLimits& limits);
    void stop_search();

    const SearchSharedState& get_shared_state();

private:
    void create_thread();

    std::vector<std::thread> native_threads;
    std::vector<SearchThread*> search_threads;
    SearchSharedState shared_state;
    GameState position_ = GameState::starting_position();
};