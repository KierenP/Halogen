#pragma once

#include "chessboard/game_state.h"
#include "search/data.h"
#include "search/search.h"

#include <condition_variable>
#include <future>
#include <latch>
#include <mutex>
#include <queue>
#include <thread>

class SearchThread
{
public:
    SearchThread(int thread_id, SearchSharedState& shared_state);

    void thread_loop();
    void terminate();

    void set_position(std::latch& latch, const GameState& position);
    void reset_new_search(std::latch& latch);
    void reset_new_game(std::latch& latch);
    void start_searching(std::latch& latch, const BasicMoveList& root_move_whitelist);
    void update_previous_search_score(std::latch& latch, Score previous_search_score);

    const SearchLocalState& get_local_state();

private:
    std::mutex mutex;
    std::condition_variable cv;
    std::queue<std::function<void()>> tasks;
    bool stop = false;

    void enqueue_task(std::function<void()> func);

    const int thread_id_;
    SearchSharedState& shared_state;
    unique_ptr_huge_page<SearchLocalState> local_state;
};

class SearchThreadPool
{
public:
    SearchThreadPool(UCI::UciOutput& uci, size_t num_threads, size_t multi_pv = 1, size_t hash_size_mb = 32);
    ~SearchThreadPool();

    SearchThreadPool(const SearchThreadPool&) = delete;
    SearchThreadPool& operator=(const SearchThreadPool&) = delete;
    SearchThreadPool(SearchThreadPool&&) = delete;
    SearchThreadPool& operator=(SearchThreadPool&&) = delete;

    void reset_new_search();
    void reset_new_game();

    void set_position(const GameState& position);
    void set_hash(int hash_size_mb);
    void set_multi_pv(int multi_pv);
    void set_chess960(bool chess960);
    void set_threads(size_t threads);
    void set_previous_search_score(Score previous_search_score);

    SearchInfoData launch_search(const SearchLimits& limits);
    void stop_search();

    const SearchSharedState& get_shared_state();

private:
    void create_thread();

    std::vector<std::thread> native_threads;
    std::vector<SearchThread*> search_threads;
    SearchSharedState shared_state;
    GameState position_ = GameState::starting_position();
};