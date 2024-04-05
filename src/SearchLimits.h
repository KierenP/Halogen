#pragma once

#include "Score.h"
#include "TimeManage.h"
#include <memory>

class iDepthChecker;
class iTimeChecker;
class iMateChecker;
class iNodeChecker;

// As each type of limit (depth, time, ...) function independently, we can use composition to combine them.

class SearchLimits
{
public:
    SearchLimits();
    ~SearchLimits();
    SearchLimits(SearchLimits&&);
    SearchLimits& operator=(SearchLimits&&);

    bool HitTimeLimit() const;
    bool HitDepthLimit(int depth) const;
    bool HitMateLimit(Score score) const;
    bool HitNodeLimit(int nodes) const;

    // Returns true if more than half of the allocated time is remaining
    bool ShouldContinueSearch() const;

    void SetTimeLimits(int maxTime, int allocatedTime);
    void SetDepthLimit(int depth);
    void SetMateLimit(int moves);
    void SetNodeLimit(uint64_t nodes);
    void SetInfinite(); // disables all other limits

    int ElapsedTime() const
    {
        return timeManager.ElapsedMs();
    }

    void ResetTimer()
    {
        timeManager.Reset();
    }

private:
    SearchTimeManage timeManager;

    std::unique_ptr<iDepthChecker> depthLimit;
    std::unique_ptr<iTimeChecker> timeLimit;
    std::unique_ptr<iMateChecker> mateLimit;
    std::unique_ptr<iNodeChecker> nodeLimit;
};
