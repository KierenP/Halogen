#pragma once

#include "bitboard/enum.h"
#include "network/inputs/king_bucket.h"
#include "network/inputs/threat.h"
#include "spsa/tuneable.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace NN
{

constexpr size_t INPUT_SIZE = 12 * 64;
constexpr size_t FT_SIZE = 640;
constexpr size_t L1_SIZE = 32;
constexpr size_t L2_SIZE = 32;

constexpr size_t OUTPUT_BUCKETS = 8;

// ============================================================
// Input layout constants
// ============================================================

// Feature layout in the weight matrix:
//   [0, 768 * KING_BUCKET_COUNT)                           : king-bucketed piece-square
//   [768 * KING_BUCKET_COUNT + 768, ...)                   : threat features

// Total number of inputs into the feature transformer:
//   king-bucketed piece-square + threat features
constexpr size_t TOTAL_FT_INPUTS = KingBucket::TOTAL_KING_BUCKET_INPUTS + Threats::TOTAL_THREAT_FEATURES;

// These quantization factors are selected to fit within certain bounds to avoid overflow while being as large as
// possible. In particular, we must avoid the following:
//  - accumulator (int16_t) overflow: round(255 * 1.98) * (32 + 1) = 16665
//  - l1 activation overflow (int16_t): (127 * round(64 * 1.98)) * 2 = 32258

constexpr int16_t FT_SCALE = 255;
constexpr int16_t L1_SCALE = 64;
TUNEABLE_CONSTANT float SCALE_FACTOR = 192.5f;

struct network
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, KingBucket::TOTAL_KING_BUCKET_INPUTS> ft_weight = {};
    alignas(64) std::array<std::array<int8_t, FT_SIZE>, Threats::TOTAL_THREAT_FEATURES> ft_threat_weight = {};
    alignas(64) std::array<int16_t, FT_SIZE> ft_bias = {};
    alignas(64) std::array<std::array<int8_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int32_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE * 2>, OUTPUT_BUCKETS> l2_weight = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l2_bias = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l3_weight = {};
    alignas(64) std::array<float, OUTPUT_BUCKETS> l3_bias = {};
};

}