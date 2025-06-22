#pragma once
#include <chrono>

using chess_clock_t = std::chrono::high_resolution_clock;

class Timer
{
public:
    Timer();

    [[nodiscard]] chess_clock_t::duration elapsed() const;
    void reset();

private:
    chess_clock_t::time_point begin_;
};

class SearchTimeManager
{
public:
    SearchTimeManager(chess_clock_t::duration soft_limit, chess_clock_t::duration hard_limit);

    bool should_continue_search(float factor) const;
    bool should_abort_search() const;

private:
    Timer timer;

    // The amount of time we have allocated to this turn. If the position is indecisive the search might extend this
    chess_clock_t::duration soft_limit_;

    // The hard limit we cannot exceed.
    chess_clock_t::duration hard_limit_;
};
