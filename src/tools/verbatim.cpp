// Load the network file, do some preprocessing, and then save it to the build directory for inclusion in the final
// binary

#include "network/arch.hpp"
#include "network/inputs/king_bucket.h"
#include "network/inputs/threat.h"
#include "network/simd/define.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <ranges>
#include <tuple>
#include <utility>

namespace NN
{

struct raw_network
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, KingBucket::TOTAL_KING_BUCKET_INPUTS> ft_weight = {};
    alignas(64) std::array<std::array<int8_t, FT_SIZE>, Threats::TOTAL_THREAT_FEATURES> ft_threat_weight = {};
    alignas(64) std::array<int16_t, FT_SIZE> ft_bias = {};
    alignas(64) std::array<std::array<std::array<int16_t, FT_SIZE>, L1_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int16_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L1_SIZE * 2>, L2_SIZE>, OUTPUT_BUCKETS> l2_weight = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l2_bias = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l3_weight = {};
    alignas(64) std::array<float, OUTPUT_BUCKETS> l3_bias = {};
};

auto shuffle_ft_neurons(const decltype(raw_network::ft_weight)& ft_weight,
    const decltype(raw_network::ft_threat_weight)& ft_threat_weight, const decltype(raw_network::ft_bias)& ft_bias,
    const decltype(raw_network::l1_weight)& l1_weight)
{
    auto ft_weight_output = std::make_unique<decltype(raw_network::ft_weight)>();
    auto ft_threat_weight_output = std::make_unique<decltype(raw_network::ft_threat_weight)>();
    auto ft_bias_output = std::make_unique<decltype(raw_network::ft_bias)>();
    auto l1_weight_output = std::make_unique<decltype(raw_network::l1_weight)>();

#ifdef NETWORK_SHUFFLE
    *ft_weight_output = ft_weight;
    *ft_threat_weight_output = ft_threat_weight;
    *ft_bias_output = ft_bias;
    *l1_weight_output = l1_weight;
#else
    // clang-format off
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = {
        151, 215, 210, 204, 53, 90, 228, 219, 32, 239, 105, 9, 21, 230, 124, 156, 84, 4, 206, 229, 248, 48, 3, 201, 31,
        117, 143, 232, 89, 59, 24, 222, 240, 47, 34, 35, 18, 172, 147, 125, 176, 208, 241, 205, 22, 7, 214, 127,
        113, 236, 139, 13, 51, 213, 11, 175, 87, 20, 45, 183, 212, 74, 223, 61, 91, 40, 67, 63, 162, 244, 145, 192,
        1, 148, 179, 246, 197, 193, 101, 135, 234, 182, 62, 132, 0, 238, 126, 81, 155, 107, 188, 64, 187, 49, 160,
        104, 5, 245, 96, 174, 122, 152, 2, 195, 190, 250, 243, 100, 186, 15, 196, 131, 46, 170, 167, 39, 164, 237,
        209, 111, 123, 103, 94, 71, 129, 130, 56, 141, 184, 52, 98, 128, 50, 173, 109, 55, 78, 233, 83, 153, 142,
        140, 58, 26, 168, 185, 118, 68, 217, 79, 144, 252, 72, 200, 93, 220, 198, 194, 158, 30, 121, 191, 69, 163,
        54, 115, 202, 8, 43, 82, 166, 14, 6, 146, 25, 16, 29, 225, 251, 60, 137, 231, 253, 159, 57, 108, 27, 95, 38,
        199, 207, 177, 165, 37, 180, 42, 157, 218, 133, 12, 224, 227, 102, 86, 10, 73, 19, 154, 247, 119, 99, 211,
        33, 221, 134, 112, 226, 255, 41, 169, 65, 76, 44, 77, 216, 242, 189, 150, 161, 17, 171, 114, 136, 110, 235,
        97, 85, 70, 23, 116, 66, 181, 92, 106, 88, 36, 80, 75, 203, 120, 149, 28, 138, 178, 254, 249,
    };
    // clang-format on

    constexpr auto is_valid_permutation = [](auto order) constexpr
    {
        std::ranges::sort(order);
        return std::ranges::equal(order, std::views::iota(size_t { 0 }, FT_SIZE / 2));
    };
    static_assert(is_valid_permutation(shuffle_order), "shuffle_order must be a valid permutation of [0, FT_SIZE / 2)");

    for (size_t i = 0; i < FT_SIZE / 2; i++)
    {
        auto old_idx = shuffle_order[i];

        (*ft_bias_output)[i] = ft_bias[old_idx];
        (*ft_bias_output)[i + FT_SIZE / 2] = ft_bias[old_idx + FT_SIZE / 2];

        for (size_t j = 0; j < KingBucket::TOTAL_KING_BUCKET_INPUTS; j++)
        {
            (*ft_weight_output)[j][i] = ft_weight[j][old_idx];
            (*ft_weight_output)[j][i + FT_SIZE / 2] = ft_weight[j][old_idx + FT_SIZE / 2];
        }

        for (size_t j = 0; j < Threats::TOTAL_THREAT_FEATURES; j++)
        {
            (*ft_threat_weight_output)[j][i] = ft_threat_weight[j][old_idx];
            (*ft_threat_weight_output)[j][i + FT_SIZE / 2] = ft_threat_weight[j][old_idx + FT_SIZE / 2];
        }

        for (size_t j = 0; j < OUTPUT_BUCKETS; j++)
        {
            for (size_t k = 0; k < L1_SIZE; k++)
            {
                (*l1_weight_output)[j][k][i] = l1_weight[j][k][old_idx];
                (*l1_weight_output)[j][k][i + FT_SIZE / 2] = l1_weight[j][k][old_idx + FT_SIZE / 2];
            }
        }
    }
#endif
    return std::make_tuple(std::move(ft_weight_output), std::move(ft_threat_weight_output), std::move(ft_bias_output),
        std::move(l1_weight_output));
}

auto adjust_for_packus(const decltype(raw_network::ft_weight)& ft_weight,
    const decltype(raw_network::ft_threat_weight)& ft_threat_weight, const decltype(raw_network::ft_bias)& ft_bias)
{
    auto permuted_ft_weight = std::make_unique<decltype(raw_network::ft_weight)>();
    auto permuted_threat_weight = std::make_unique<decltype(raw_network::ft_threat_weight)>();
    auto permuted_bias = std::make_unique<decltype(raw_network::ft_bias)>();

#if defined(USE_AVX2)
#if defined(USE_AVX512)
    constexpr std::array<size_t, 8> mapping = { 0, 4, 1, 5, 2, 6, 3, 7 };
#else
    constexpr std::array<size_t, 4> mapping = { 0, 2, 1, 3 };
#endif

    // Permute FT weight
    for (size_t i = 0; i < ft_weight.size(); i++)
    {
        for (size_t j = 0; j < FT_SIZE; j += SIMD::vec_size)
        {
            for (size_t x = 0; x < SIMD::vec_size; x++)
            {
                size_t target_idx = j + mapping[x / 8] * 8 + x % 8;
                (*permuted_ft_weight)[i][target_idx] = ft_weight[i][j + x];
            }
        }
    }

    // Permute threat weight
    for (size_t i = 0; i < ft_threat_weight.size(); i++)
    {
        for (size_t j = 0; j < FT_SIZE; j += SIMD::vec_size)
        {
            for (size_t x = 0; x < SIMD::vec_size; x++)
            {
                size_t target_idx = j + mapping[x / 8] * 8 + x % 8;
                (*permuted_threat_weight)[i][target_idx] = ft_threat_weight[i][j + x];
            }
        }
    }

    // Permute bias
    for (size_t j = 0; j < FT_SIZE; j += SIMD::vec_size)
    {
        for (size_t x = 0; x < SIMD::vec_size; x++)
        {
            size_t target_idx = j + mapping[x / 8] * 8 + x % 8;
            (*permuted_bias)[target_idx] = ft_bias[j + x];
        }
    }

#else
    (*permuted_ft_weight) = ft_weight;
    (*permuted_threat_weight) = ft_threat_weight;
    (*permuted_bias) = ft_bias;
#endif

    return std::make_tuple(std::move(permuted_ft_weight), std::move(permuted_threat_weight), std::move(permuted_bias));
}

auto rescale_l1_bias(const decltype(raw_network::l1_bias)& input)
{
    auto output = std::make_unique<decltype(raw_network::l1_bias)>();

    // rescale l1 bias to account for FT_activation adjusting to 0-127 scale
    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            (*output)[i][j] = input[i][j] * 127 / FT_SCALE;
        }
    }

    return output;
}

auto interleave_for_l1_sparsity(const decltype(raw_network::l1_weight)& input)
{
    auto output = std::make_unique<std::array<std::array<int16_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS>>();

#if defined(SIMD_ENABLED)
    // transpose and interleave the weights in blocks of 4 FT activations
    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            for (size_t k = 0; k < FT_SIZE; k++)
            {
                // we want something that looks like:
                //[bucket][nibble][output][4]
                (*output)[i][(k / 4) * (4 * L1_SIZE) + (j * 4) + (k % 4)] = input[i][j][k];
            }
        }
    }
#else
    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            for (size_t k = 0; k < FT_SIZE; k++)
            {
                (*output)[i][j * FT_SIZE + k] = input[i][j][k];
            }
        }
    }
#endif
    return output;
}

auto cast_l1_weight_int8(const std::array<std::array<int16_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS>& input)
{
    auto output = std::make_unique<std::array<std::array<int8_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < FT_SIZE * L1_SIZE; j++)
        {
            (*output)[i][j] = static_cast<int8_t>(input[i][j]);
        }
    }

    return output;
}

auto cast_l1_bias_int32(const decltype(raw_network::l1_bias)& input)
{
    auto output = std::make_unique<std::array<std::array<int32_t, L1_SIZE>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            (*output)[i][j] = static_cast<int32_t>(input[i][j]);
        }
    }

    return output;
}

auto transpose_l2_weights(const decltype(raw_network::l2_weight)& input)
{
    auto output = std::make_unique<std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE * 2>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE * 2; j++)
        {
            for (size_t k = 0; k < L2_SIZE; k++)
            {
                (*output)[i][j][k] = input[i][k][j];
            }
        }
    }

    return output;
}

}

using namespace NN;

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << "Error: expected 2 command line arguments <input> and <output>" << std::endl;
        return EXIT_FAILURE;
    }

    if (auto size = std::filesystem::file_size(argv[1]); size != sizeof(raw_network))
    {
        std::cout << "Error: expected to read " << sizeof(raw_network) << " bytes but file size is " << size << " bytes"
                  << std::endl;
        return EXIT_FAILURE;
    }

    auto raw_net = std::make_unique<raw_network>();
    std::ifstream in(argv[1], std::ios::binary);
    in.read(reinterpret_cast<char*>(raw_net.get()), sizeof(raw_network));

    auto [ft_weight, ft_threat_weight, ft_bias, l1_weight]
        = shuffle_ft_neurons(raw_net->ft_weight, raw_net->ft_threat_weight, raw_net->ft_bias, raw_net->l1_weight);
    auto [ft_weight2, ft_threat_weight2, ft_bias2] = adjust_for_packus(*ft_weight, *ft_threat_weight, *ft_bias);
    auto final_net = std::make_unique<network>();
    final_net->ft_weight = *ft_weight2;
    final_net->ft_threat_weight = *ft_threat_weight2;
    final_net->ft_bias = *ft_bias2;
    final_net->l1_weight = *cast_l1_weight_int8(*interleave_for_l1_sparsity(*l1_weight));
    final_net->l1_bias = *cast_l1_bias_int32(*rescale_l1_bias(raw_net->l1_bias));
    final_net->l2_weight = *transpose_l2_weights(raw_net->l2_weight);
    final_net->l2_bias = raw_net->l2_bias;
    final_net->l3_weight = raw_net->l3_weight;
    final_net->l3_bias = raw_net->l3_bias;

    std::ofstream out(argv[2], std::ios::binary);
    out.write(reinterpret_cast<const char*>(final_net.get()), sizeof(network));

    std::cout << "Created embedded network" << std::endl;
}