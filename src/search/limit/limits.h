#pragma once

#include "search/limit/time.h"

#include <cstdint>
#include <optional>

class SearchLimits
{
public:
    std::optional<SearchTimeManager> time;
    std::optional<int> depth;
    std::optional<int> mate;
    std::optional<uint64_t> nodes;
};
