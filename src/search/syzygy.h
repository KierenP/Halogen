#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "movegen/move.h"
#include "search/data.h"
#include "search/score.h"
#include "utility/static_vector.h"

class BoardState;

class RootProbeResult
{
public:
    struct RootMove
    {
        Move move;
        int32_t tb_rank;
    };

    StaticVector<RootMove, MAX_LEGAL_MOVES> root_moves;
};

class Syzygy
{
public:
    static void init(std::string_view path, bool print);

    static std::optional<Score> probe_wdl_search(const SearchLocalState& local, int distance_from_root);
    static std::optional<RootProbeResult> probe_dtz_root(const GameState& position);
};