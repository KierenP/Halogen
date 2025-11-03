#include "search/limit/time.h"

#include "spsa/tuneable.h"

#include <chrono>
#include <compare>
#include <ratio>

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

bool SearchTimeManager::should_continue_search(const Timer& timer, float factor) const
{
    // if AllocatedSearchTimeMS == MaxTimeMS then we have recieved a 'go movetime X' command and we should not abort
    // search early
    if (soft_limit_ == hard_limit_)
    {
        return true;
    }

    auto elapsed_ms = timer.elapsed();
    return (elapsed_ms < soft_limit_ * factor * soft_tm && elapsed_ms < hard_limit_);
}

bool SearchTimeManager::should_abort_search(const Timer& timer) const
{
    auto elapsed_ms = timer.elapsed();
    return (elapsed_ms >= soft_limit_ || elapsed_ms >= hard_limit_);
}
