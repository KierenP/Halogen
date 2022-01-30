#include "TimeManage.h"

#include <algorithm>

int Timer::ElapsedMs() const
{
    return (get_time_point() - Begin);
}

SearchTimeManage::SearchTimeManage(int maxTime, int allocatedTime)
    : AllocatedSearchTimeMS(allocatedTime)
    , MaxTimeMS(maxTime)
{
}

bool SearchTimeManage::ContinueSearch() const
{
    //if AllocatedSearchTimeMS == MaxTimeMS then we have recieved a 'go movetime X' command and we should not abort search early
    return (AllocatedSearchTimeMS == MaxTimeMS || timer.ElapsedMs() < AllocatedSearchTimeMS / 2);
}

bool SearchTimeManage::AbortSearch() const
{
    return (timer.ElapsedMs() > (std::min)(AllocatedSearchTimeMS, MaxTimeMS - BufferTime));
}
