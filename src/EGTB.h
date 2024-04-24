#pragma once

#include "BoardState.h"
#include "Move.h"
#include "MoveList.h"
#include "Score.h"
#include "StaticVector.h"

#include <optional>
#include <string_view>

// models a Pyrrhic TB_RESULT and converts move/score to our domain types
class TbResult
{
public:
    TbResult() {};
    TbResult(uint32_t result);

    Score get_score(int distance_from_root) const;
    Move get_move(const BoardState& board) const;

private:
    uint32_t result_;
};

class RootProbeResult
{
public:
    TbResult root_result_;
    BasicMoveList root_move_whitelist;
};

class Syzygy
{
public:
    static void init(std::string_view path);

    static std::optional<TbResult> probe_wdl_search(const BoardState& board);
    static std::optional<RootProbeResult> probe_dtz_root(const BoardState& board);
};