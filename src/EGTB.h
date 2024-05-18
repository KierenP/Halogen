#pragma once

#include "BoardState.h"
#include "Move.h"
#include "MoveList.h"
#include "Score.h"

#include <optional>
#include <string_view>

class RootProbeResult
{
public:
    BasicMoveList root_move_whitelist;
};

class Syzygy
{
public:
    static void init(std::string_view path);

    static std::optional<Score> probe_wdl_search(const BoardState& board, int distance_from_root);
    static std::optional<RootProbeResult> probe_dtz_root(const BoardState& board);
};