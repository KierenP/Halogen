#include "SearchLimits.h"
#include "BitBoardDefine.h"
#include <memory>

// DepthChecker component. Will compare the depth to the set limit. Obeys the global max depth limit as well.

class iDepthChecker
{
public:
    virtual ~iDepthChecker() = default;
    virtual bool HitDepthLimit(int depth) const = 0;
};

class NullDepthChecker : public iDepthChecker
{
public:
    bool HitDepthLimit(int) const override { return false; }
};

class DepthChecker : public iDepthChecker
{
    int depthLimit_;

public:
    DepthChecker(int depthLimit)
        : depthLimit_(std::min(MAX_DEPTH, depthLimit))
    {
    }

    bool HitDepthLimit(int depth) const override
    {
        return depth > depthLimit_;
    }
};

// TimeChecker component. Will compare the elapsed time to the set limit. Relies on a externally managed timer.

class iTimeChecker
{
public:
    virtual ~iTimeChecker() = default;
    virtual bool HitTimeLimit(const SearchTimeManage&) const = 0;
    virtual bool ShouldContinueSearch(const SearchTimeManage&) const = 0;
};

class NullTimeChecker : public iTimeChecker
{
public:
    bool HitTimeLimit(const SearchTimeManage&) const override { return false; }
    bool ShouldContinueSearch(const SearchTimeManage&) const override { return true; };
};

class TimeChecker : public iTimeChecker
{
public:
    bool HitTimeLimit(const SearchTimeManage& timer) const override { return timer.AbortSearch(); }
    bool ShouldContinueSearch(const SearchTimeManage& timer) const override { return timer.ContinueSearch(); };
};

// MateChecker component. Will compare the score to see if we found a sufficently fast mate.

class iMateChecker
{
public:
    virtual ~iMateChecker() = default;
    virtual bool HitMateLimit(int score) const = 0;
};

class NullMateChecker : public iMateChecker
{
public:
    bool HitMateLimit(int) const override { return false; };
};

class MateChecker : public iMateChecker
{
    int mateLimit_;

public:
    MateChecker(int mateLimit)
        : mateLimit_(mateLimit)
    {
    }

    bool HitMateLimit(int score) const override { return (MATE)-abs(score) <= 2 * mateLimit_; };
};

// Now put all the pieces together into a single object.

SearchLimits::SearchLimits()
    : depthLimit(std::make_unique<NullDepthChecker>())
    , timeLimit(std::make_unique<NullTimeChecker>())
    , mateLimit(std::make_unique<NullMateChecker>())
{
}

SearchLimits::~SearchLimits() = default;
SearchLimits::SearchLimits(SearchLimits&&) = default;
SearchLimits& SearchLimits::operator=(SearchLimits&&) = default;

bool SearchLimits::HitTimeLimit() const
{
    return timeLimit->HitTimeLimit(timeManager);
}

bool SearchLimits::HitDepthLimit(int depth) const
{
    return depthLimit->HitDepthLimit(depth);
}

bool SearchLimits::HitMateLimit(int score) const
{
    return mateLimit->HitMateLimit(score);
}

bool SearchLimits::ShouldContinueSearch() const
{
    return timeLimit->ShouldContinueSearch(timeManager);
}

void SearchLimits::SetInfinite()
{
    depthLimit = std::make_unique<NullDepthChecker>();
    timeLimit = std::make_unique<NullTimeChecker>();
    mateLimit = std::make_unique<NullMateChecker>();
}

void SearchLimits::SetTimeLimits(int maxTime, int allocatedTime)
{
    timeManager = SearchTimeManage(maxTime, allocatedTime);
    timeLimit = std::make_unique<TimeChecker>();
}

void SearchLimits::SetDepthLimit(int depth)
{
    depthLimit = std::make_unique<DepthChecker>(depth);
}

void SearchLimits::SetMateLimit(int moves)
{
    mateLimit = std::make_unique<MateChecker>(moves);
}