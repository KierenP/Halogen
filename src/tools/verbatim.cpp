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
        257, 212, 191, 141, 39, 73, 280, 93, 77, 298, 128, 293, 67, 157, 174, 56, 175, 155, 284, 271, 158, 34, 14, 258,
        45, 161, 24, 268, 91, 108, 202, 81, 249, 84, 314, 134, 79, 194, 259, 205, 139, 316, 218, 154, 90, 265, 163,
        199, 234, 83, 196, 182, 49, 256, 241, 315, 76, 156, 309, 263, 159, 3, 235, 33, 136, 117, 105, 143, 215, 318,
        246, 27, 23, 58, 219, 17, 1, 160, 225, 32, 129, 57, 260, 208, 306, 252, 217, 43, 300, 239, 262, 281, 210,
        96, 88, 69, 146, 162, 5, 209, 126, 211, 38, 44, 285, 0, 123, 177, 130, 183, 98, 142, 125, 149, 112, 261, 42,
        264, 290, 119, 178, 121, 216, 13, 317, 195, 120, 227, 50, 115, 54, 16, 132, 170, 255, 37, 276, 41, 30, 111,
        166, 266, 10, 275, 63, 313, 214, 15, 185, 31, 150, 228, 201, 8, 223, 59, 74, 110, 36, 122, 186, 288, 92,
        137, 107, 197, 180, 94, 287, 9, 148, 168, 167, 237, 173, 102, 198, 251, 303, 26, 179, 304, 85, 109, 295, 86,
        64, 187, 245, 244, 230, 61, 176, 18, 282, 169, 291, 22, 68, 131, 181, 299, 192, 72, 286, 35, 153, 207, 144,
        296, 222, 82, 145, 80, 152, 51, 278, 46, 204, 189, 307, 60, 52, 101, 2, 269, 243, 71, 311, 75, 133, 254,
        294, 62, 12, 188, 224, 104, 238, 277, 229, 78, 308, 292, 47, 97, 253, 95, 138, 319, 206, 164, 140, 89, 190,
        203, 135, 127, 28, 29, 312, 226, 279, 240, 48, 171, 25, 106, 232, 267, 184, 113, 213, 283, 66, 165, 231, 40,
        247, 172, 87, 272, 124, 7, 248, 221, 53, 147, 55, 21, 4, 151, 70, 242, 114, 103, 65, 236, 200, 270, 220,
        274, 273, 301, 6, 193, 233, 100, 19, 116, 310, 250, 305, 289, 11, 20, 118, 302, 99, 297,
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