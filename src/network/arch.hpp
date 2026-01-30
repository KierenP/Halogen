#pragma once

#include "bitboard/enum.h"
#include "spsa/tuneable.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

constexpr size_t INPUT_SIZE = 12 * 64;
constexpr size_t FT_SIZE = 1536;
constexpr size_t L1_SIZE = 16;
constexpr size_t L2_SIZE = 32;

constexpr size_t OUTPUT_BUCKETS = 8;

// clang-format off
constexpr std::array<size_t, N_SQUARES> KING_BUCKETS = {
    0, 1, 2, 3, 3, 2, 1, 0,
    4, 4, 5, 5, 5, 5, 4, 4,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
};
// clang-format on

constexpr size_t KING_BUCKET_COUNT = []()
{
    auto [min, max] = std::minmax_element(KING_BUCKETS.begin(), KING_BUCKETS.end());
    return *max - *min + 1;
}();

// These quantization factors are selected to fit within certain bounds to avoid overflow while being as large as
// possible. In particular, we must avoid the following:
//  - accumulator (int16_t) overflow: round(255 * 1.98) * (32 + 1) = 16665
//  - l1 activation overflow (int16_t): (127 * round(64 * 1.98)) * 2 = 32258

constexpr int16_t FT_SCALE = 255;
constexpr int16_t L1_SCALE = 64;
TUNEABLE_CONSTANT float SCALE_FACTOR = 192.5f;

namespace NN
{

struct network
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, INPUT_SIZE * KING_BUCKET_COUNT> ft_weight = {};
    alignas(64) std::array<int16_t, FT_SIZE> ft_bias = {};
    alignas(64) std::array<std::array<int8_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int32_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE * 2>, OUTPUT_BUCKETS> l2_weight = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l2_bias = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l3_weight = {};
    alignas(64) std::array<float, OUTPUT_BUCKETS> l3_bias = {};
};

}