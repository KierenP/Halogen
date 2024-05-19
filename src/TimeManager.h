#pragma once
#include <atomic>
#include <chrono>
#include <time.h>

using chess_clock_t = std::chrono::high_resolution_clock;

// TODO: move this into SearchSharedState
inline std::atomic<bool> KeepSearching;

class Timer
{
public:
    Timer();

    chess_clock_t::duration elapsed() const;
    void reset();

private:
    chess_clock_t::time_point begin_;
};

class SearchTimeManager
{
public:
    SearchTimeManager(chess_clock_t::duration soft_limit, chess_clock_t::duration hard_limit);

    bool ShouldContinueSearch() const;
    bool ShouldAbortSearch() const;

private:
    Timer timer;

    // The amount of time we have allocated to this turn. If the position is indecisive the search might extend this
    chess_clock_t::duration soft_limit_;

    // The hard limit we cannot exceed.
    chess_clock_t::duration hard_limit_;
};
