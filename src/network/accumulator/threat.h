#pragma once

#include "bitboard/define.h"
#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "movegen/movegen.h"
#include "network/arch.hpp"
#include "network/inputs/threat.h"
#include "network/simd/accumulator.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace NN::Threats
{

// A single threat, stored as the original attacker->victim description. w_king and b_king are same for all
// ThreatDeltas, so they are stored in the accumulator
struct ThreatDelta
{
    Piece atk_pt;
    Square atk_sq;
    Piece vic_pt;
    Square vic_sq;
};

struct ThreatIndices
{
    uint32_t white_idx;
    uint32_t black_idx;
};

// Threat input accumulator. Stores threat feature contributions per side (WHITE/BLACK perspective).
// Incrementally updated via threat deltas computed in store_lazy_updates.
struct ThreatAccumulator
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, N_SIDES> side = {};

    bool operator==(const ThreatAccumulator& rhs) const;

    // Maximum number of threat deltas per move. A move changes at most 4 squares (castling),
    // each square can be involved in threats as attacker and victim, plus discovered attacks.
    static constexpr size_t MAX_THREAT_DELTAS = 256;

    // data for lazy updates
    bool white_threats_requires_recalculation = false;
    bool black_threats_requires_recalculation = false;
    bool acc_is_valid = false;

    // Threat deltas: features to add and subtract from the previous accumulator
    Square w_king;
    Square b_king;
    std::array<ThreatDelta, MAX_THREAT_DELTAS> threat_adds = {};
    size_t n_threat_adds = 0;
    std::array<ThreatDelta, MAX_THREAT_DELTAS> threat_subs = {};
    size_t n_threat_subs = 0;

    void store_lazy_updates(
        const BoardState& prev_board, const BoardState& post_board, uint64_t sub_bb, uint64_t add_bb);

    // Compute all threats from scratch for a given board position.
    void recalculate_from_scratch(const BoardState& board, const NN::network& net);
    void recalculate_side_from_scratch(const BoardState& board, const NN::network& net, Side perspective);

    // Apply stored threat deltas to produce the new threat accumulator from the previous one.
    void apply_lazy_updates(const ThreatAccumulator& prev, const BoardState& board, const NN::network& net);
};

}