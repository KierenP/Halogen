#pragma once

#include <cstdint>
#include <optional>

#include "TimeManager.h"

class SearchLimits
{
public:
    std::optional<SearchTimeManager> time;
    std::optional<int> depth;
    std::optional<int> mate;
    std::optional<uint64_t> nodes;
};
