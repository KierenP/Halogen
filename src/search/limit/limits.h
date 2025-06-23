#pragma once

#include <cstdint>
#include <optional>

#include "search/limit/time.h"

class SearchLimits
{
public:
    std::optional<SearchTimeManager> time;
    std::optional<int> depth;
    std::optional<int> mate;
    std::optional<uint64_t> nodes;
};
