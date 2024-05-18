#pragma once

#include "Score.h"
#include "TimeManager.h"

#include <optional>

class SearchLimits
{
public:
    std::optional<SearchTimeManager> time;
    std::optional<int> depth;
    std::optional<int> mate;
    std::optional<uint64_t> nodes;
};
