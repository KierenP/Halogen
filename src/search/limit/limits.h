#pragma once

#include "movegen/list.h"
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
    // Legal moves to restrict the root to. Empty means search all moves
    BasicMoveList search_moves;
};
