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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 212, 206, 268, 3, 256, 298, 153, 0, 100, 287, 216, 123,
        240, 157, 235, 286, 45, 172, 109, 220, 5, 156, 176, 265, 51, 190, 174, 77, 107, 307, 253, 252, 117, 305, 288,
        67, 178, 86, 155, 284, 73, 203, 143, 13, 6, 7, 260, 177, 169, 38, 124, 66, 290, 259, 149, 217, 46, 276, 280, 56,
        262, 292, 55, 128, 65, 12, 251, 312, 9, 207, 257, 197, 142, 61, 137, 64, 238, 79, 26, 296, 266, 42, 72, 281,
        121, 180, 191, 226, 131, 237, 316, 81, 263, 105, 98, 148, 36, 267, 95, 43, 160, 35, 183, 21, 22, 82, 78, 85,
        275, 161, 88, 126, 30, 232, 303, 242, 182, 140, 170, 108, 60, 92, 122, 301, 205, 248, 291, 152, 53, 306, 173,
        93, 215, 58, 258, 63, 221, 91, 229, 34, 40, 204, 244, 318, 57, 48, 39, 299, 118, 246, 104, 168, 208, 310, 147,
        247, 239, 87, 198, 270, 141, 11, 186, 163, 20, 165, 138, 134, 192, 162, 171, 97, 150, 96, 31, 69, 179, 159, 135,
        187, 120, 269, 214, 293, 101, 139, 119, 146, 144, 181, 1, 50, 130, 233, 52, 194, 33, 219, 241, 264, 200, 145,
        62, 25, 41, 17, 319, 110, 151, 133, 255, 127, 16, 76, 302, 295, 234, 37, 184, 103, 99, 202, 90, 211, 201, 19,
        27, 227, 249, 83, 230, 102, 261, 218, 132, 154, 166, 15, 222, 164, 297, 188, 236, 129, 209, 294, 94, 70, 18,
        304, 116, 195, 68, 44, 300, 277, 89, 213, 225, 111, 28, 125, 282, 313, 24, 115, 196, 54, 71, 80, 113, 271, 272,
        175, 273, 136, 167, 315, 274, 314, 250, 308, 210, 189, 59, 199, 283, 32, 84, 29, 75, 254, 158, 4, 106, 185, 74,
        285, 231, 49, 289, 10, 114, 278, 193, 2, 224, 228, 245, 47, 23, 309, 8, 243, 112, 14, 279, 317, 223, 311,
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