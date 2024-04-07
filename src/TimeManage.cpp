#include "TimeManage.h"

#include <algorithm>

int Timer::ElapsedMs() const
{
    return (get_time_point() - Begin);
}

SearchTimeManage::SearchTimeManage(int soft_limit, int hard_limit)
    : soft_limit_(soft_limit)
    , hard_limit_(hard_limit)
{
}

bool SearchTimeManage::ContinueSearch() const
{
    // if AllocatedSearchTimeMS == MaxTimeMS then we have recieved a 'go movetime X' command and we should not abort
    // search early
    if (soft_limit_ == hard_limit_)
    {
        return true;
    }

    auto elapsed_ms = timer.ElapsedMs();

    return (elapsed_ms < soft_limit_ / 2 && elapsed_ms < hard_limit_);
}

bool SearchTimeManage::AbortSearch() const
{
    auto elapsed_ms = timer.ElapsedMs();
    return (elapsed_ms > soft_limit_ && elapsed_ms > hard_limit_);
}
