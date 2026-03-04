#include "piece_count.h"

#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "network/inputs/piece_count.h"
#include "network/simd/accumulator.hpp"

#include <cassert>

namespace NN::PieceCount
{

bool PieceCountAccumulator::operator==(const PieceCountAccumulator& rhs) const
{
    return side == rhs.side;
}

// Helper: add all active features for a perspective
static void add_all_features(std::array<int16_t, FT_SIZE>& acc_side,
    const std::array<std::array<uint8_t, N_PIECE_TYPES>, N_SIDES>& piece_counts, Side perspective,
    const NN::network& net)
{
    // "my" pieces = perspective's pieces → feature indices [0..15)
    // "opponent's" pieces = !perspective's pieces → feature indices [15..30)
    emit_features_for_side(
        piece_counts[perspective], 0, [&](size_t idx) { NN::add1(acc_side, net.ft_piece_count_weight[idx]); });
    emit_features_for_side(piece_counts[!perspective], PIECE_COUNT_PER_SIDE,
        [&](size_t idx) { NN::add1(acc_side, net.ft_piece_count_weight[idx]); });
}

void PieceCountAccumulator::recalculate_from_scratch(const BoardState& board, const NN::network& net)
{
    *this = {};
    counts = compute_piece_counts(board);

    add_all_features(side[WHITE], counts, WHITE, net);
    add_all_features(side[BLACK], counts, BLACK, net);

    acc_is_valid = true;
}

// Helper: given old and new piece counts, sub old features and add new features for one perspective
static void update_perspective(std::array<int16_t, FT_SIZE>& acc_side,
    const std::array<std::array<uint8_t, N_PIECE_TYPES>, N_SIDES>& old_counts,
    const std::array<std::array<uint8_t, N_PIECE_TYPES>, N_SIDES>& new_counts, Side perspective, const NN::network& net)
{
    // For each side's pieces, check if any thresholds changed
    // Rather than tracking individual threshold crossings, we can diff the active feature sets.
    // Since there are at most 15 features per side (30 total), this is cheap.

    auto emit_and_collect = [](const std::array<uint8_t, N_PIECE_TYPES>& counts, size_t base,
                                std::array<size_t, PIECE_COUNT_PER_SIDE>& features, size_t& n)
    {
        n = 0;
        emit_features_for_side(counts, base, [&](size_t idx) { features[n++] = idx; });
    };

    // Collect old features
    std::array<size_t, PIECE_COUNT_PER_SIDE> old_my_features {};
    size_t old_my_count = 0;
    std::array<size_t, PIECE_COUNT_PER_SIDE> old_opp_features {};
    size_t old_opp_count = 0;
    emit_and_collect(old_counts[perspective], 0, old_my_features, old_my_count);
    emit_and_collect(old_counts[!perspective], PIECE_COUNT_PER_SIDE, old_opp_features, old_opp_count);

    // Collect new features
    std::array<size_t, PIECE_COUNT_PER_SIDE> new_my_features {};
    size_t new_my_count = 0;
    std::array<size_t, PIECE_COUNT_PER_SIDE> new_opp_features {};
    size_t new_opp_count = 0;
    emit_and_collect(new_counts[perspective], 0, new_my_features, new_my_count);
    emit_and_collect(new_counts[!perspective], PIECE_COUNT_PER_SIDE, new_opp_features, new_opp_count);

    // Build a simple bitmap of which features are active (30 features fit in a uint32_t)
    uint32_t old_active = 0;
    uint32_t new_active = 0;

    for (size_t i = 0; i < old_my_count; i++)
        old_active |= (1u << old_my_features[i]);
    for (size_t i = 0; i < old_opp_count; i++)
        old_active |= (1u << old_opp_features[i]);
    for (size_t i = 0; i < new_my_count; i++)
        new_active |= (1u << new_my_features[i]);
    for (size_t i = 0; i < new_opp_count; i++)
        new_active |= (1u << new_opp_features[i]);

    // Features to remove: in old but not in new
    uint32_t to_remove = old_active & ~new_active;
    // Features to add: in new but not in old
    uint32_t to_add = new_active & ~old_active;

    while (to_remove)
    {
        int idx = __builtin_ctz(to_remove);
        to_remove &= to_remove - 1;
        NN::sub1(acc_side, net.ft_piece_count_weight[idx]);
    }

    while (to_add)
    {
        int idx = __builtin_ctz(to_add);
        to_add &= to_add - 1;
        NN::add1(acc_side, net.ft_piece_count_weight[idx]);
    }
}

void PieceCountAccumulator::store_lazy_updates(const BoardState& prev_board, const BoardState& post_board, Move move)
{
    acc_is_valid = false;

    // Compute new piece counts from the post-move board
    counts = compute_piece_counts(post_board);
}

void PieceCountAccumulator::apply_lazy_updates(const PieceCountAccumulator& prev, const NN::network& net)
{
    // If counts haven't changed, just copy
    if (prev.counts == counts)
    {
        side = prev.side;
        acc_is_valid = true;
        return;
    }

    // Start from previous accumulator and apply deltas
    side = prev.side;

    update_perspective(side[WHITE], prev.counts, counts, WHITE, net);
    update_perspective(side[BLACK], prev.counts, counts, BLACK, net);

    acc_is_valid = true;
}

} // namespace NN::PieceCount
