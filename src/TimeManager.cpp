#include "TimeManager.h"

#include <chrono>

Timer::Timer()
{
    reset();
}

std::chrono::nanoseconds Timer::elapsed() const
{
    auto now = std::chrono::high_resolution_clock::now();
    return (now - begin_);
}

void Timer::reset()
{
    begin_ = std::chrono::high_resolution_clock::now();
}

SearchTimeManager::SearchTimeManager(chess_clock_t::duration soft_limit, chess_clock_t::duration hard_limit)
    : soft_limit_(soft_limit)
    , hard_limit_(hard_limit)
{
}

bool SearchTimeManager::ShouldContinueSearch() const
{
    // if AllocatedSearchTimeMS == MaxTimeMS then we have recieved a 'go movetime X' command and we should not abort
    // search early
    if (soft_limit_ == hard_limit_)
    {
        return true;
    }

    auto elapsed_ms = timer.elapsed();
    return (elapsed_ms < soft_limit_ / 2 && elapsed_ms < hard_limit_);
}

bool SearchTimeManager::ShouldAbortSearch() const
{
    auto elapsed_ms = timer.elapsed();
    return (elapsed_ms >= soft_limit_ || elapsed_ms >= hard_limit_);
}
