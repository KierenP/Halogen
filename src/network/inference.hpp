#pragma once

#include "bitboard/define.h"
#include "network/simd/definitions.hpp"
#include "network/simd/utility.hpp"
#include "tools/sparse_shuffle.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

// These quantization factors are selected to fit within certain bounds to avoid overflow while being as large as
// possible. In particular, we must avoid the following:
//  - accumulator (int16_t) overflow: round(255 * 1.98) * (32 + 1) = 16665
//  - l1 activation overflow (int16_t): (127 * round(64 * 1.98)) * 2 = 32258
constexpr int16_t FT_SCALE = 255;
constexpr int16_t L1_SCALE = 64;
constexpr double SCALE_FACTOR = 160;

// For the expensive FT -> L1 matmul, we want to do a sparse multiplication. The naive approach would be to construct a
// vector of non zero FT activations, and then multiply by the corresponding L1 weights. Because we want to accumulate
// u8*i8 into i32 using two madd instructions, it makes it much easier if we work in groups of 4 FT activations at a
// time (called a FT nibble). Hence, we check blocks of 4 adjacent FT activations in a stride and omit any blocks of 4
// zeros.
//
// In order to efficiently encode the sparse FT nibbles, we process them in blocks of 8 at a time (8 * 4 = 32). Using a
// movemask instruction along with a greater than, we can extract one byte at a time to represent the sparsity of 8 FT
// nibbles. We precompute a 256 entry lookup table to map to the corresponding mask. For example, if we the mask is
// 0b10001001, then we would extract indicies = { 0, 3, 7 }. We store each index in a u16, so that 8 of them occupy
// a _m128i register. This allows us to very quickly apply a offset using a add_epi16 to efficiently map the indicies to
// the correct FT nibbles.
struct SparseAffineEntry
{
    alignas(16) std::array<uint16_t, 8> indicies;
    size_t count;
};

struct SparseAffineTable
{
    SparseAffineTable()
    {
        for (uint64_t i = 0; i < 256; i++)
        {
            uint64_t bits = i;
            size_t idx = 0;
            while (bits)
            {
                entry[i].indicies[idx++] = lsbpop(bits);
            }
            entry[i].count = idx;
        }
    }

    std::array<SparseAffineEntry, 256> entry;
};

namespace NN::Features
{

const static SparseAffineTable sparse_affine_table;

void FT_activation(const std::array<int16_t, FT_SIZE>& stm, const std::array<int16_t, FT_SIZE>& nstm,
    std::array<uint8_t, FT_SIZE>& output, [[maybe_unused]] std::array<int16_t, FT_SIZE / 4>& sparse_nibbles,
    [[maybe_unused]] size_t& sparse_nibbles_size)
{
#if defined(SIMD_ENABLED)
    // manually unrolled and interleaved x2, 13 max registers in use
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert((FT_SIZE / 2) % (stride * 4) == 0);
    const auto zero = SIMD::setzero_si();
    const auto one = SIMD::set1_epi16(FT_SCALE);
    auto sparse_nibble_offset = _mm_set1_epi16(0);
    const auto sparse_nibble_offset_adj = _mm_set1_epi16(8);
    for (size_t i = 0; i < FT_SIZE / 2; i += stride * 4)
    {
        // Idea from Alexandria, we want to calculate the screlu using int16 without overflowing. In order to
        // achieve this we use mulhi_epi16 to calculate a temporary int32 product, before extracting the high 16
        // bits. With an appropriate initial left shift, 255 * 255 * 2^7 / 2^16 = 127.00 we can calculate the screlu
        // and then get a value that fits into a int8 for the dense layer. This value when multiplied by the weights
        // will give a result of 127.00 * 64 * 1.98 = 16093, hence the maddubs_epi16 will not overflow.

        // Another clever trick from Alexandria is we can skip two max_epi16 calls (one on each pairwise mul). This
        // is because mulhi_epi16 preserves the sign of the multiplication, meaning that after the packus we get the
        // correct result in all cases. For this to work, we need to make sure the int32 won't overflow, with
        // (-255) * 1.98 * (32+1) * 255 * 2^7 = -500M we are safe.

        auto stm_vec1 = SIMD::min_epi16(one, SIMD::load_si(&stm[i]));
        auto stm_vec2 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + stride]));
        auto stm_vec3 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + stride * 2]));
        auto stm_vec4 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + stride * 3]));
        stm_vec1 = SIMD::max_epi16(zero, stm_vec1);
        stm_vec2 = SIMD::max_epi16(zero, stm_vec2);
        stm_vec3 = SIMD::max_epi16(zero, stm_vec3);
        stm_vec4 = SIMD::max_epi16(zero, stm_vec4);
        stm_vec1 = SIMD::slli_epi16(stm_vec1, 7);
        stm_vec2 = SIMD::slli_epi16(stm_vec2, 7);
        stm_vec3 = SIMD::slli_epi16(stm_vec3, 7);
        stm_vec4 = SIMD::slli_epi16(stm_vec4, 7);
        auto stm_vec5 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + FT_SIZE / 2]));
        auto stm_vec6 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + FT_SIZE / 2 + stride]));
        auto stm_vec7 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + FT_SIZE / 2 + stride * 2]));
        auto stm_vec8 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + FT_SIZE / 2 + stride * 3]));
        stm_vec1 = SIMD::mulhi_epi16(stm_vec1, stm_vec5);
        stm_vec2 = SIMD::mulhi_epi16(stm_vec2, stm_vec6);
        stm_vec3 = SIMD::mulhi_epi16(stm_vec3, stm_vec7);
        stm_vec4 = SIMD::mulhi_epi16(stm_vec4, stm_vec8);

        // We permute the weights at startup to match the packus.
        stm_vec1 = SIMD::packus_epi16(stm_vec1, stm_vec2);
        stm_vec3 = SIMD::packus_epi16(stm_vec3, stm_vec4);
        SIMD::store_si(&output[i], stm_vec1);
        SIMD::store_si(&output[i + stride * 2], stm_vec3);

        // Cache the nonzero nibbles for sparse affine l1
        auto mask1 = SIMD::cmpgt_epi32_mask(stm_vec1);
        auto mask2 = SIMD::cmpgt_epi32_mask(stm_vec3);
#if defined(USE_AVX512)
        uint32_t mask = mask1 | (mask2 << 16);
#elif defined(USE_AVX2)
        uint16_t mask = mask1 | (mask2 << 8);
#else
        uint8_t mask = mask1 | (mask2 << 4);
#endif
        // 1 byte for SSE, 2 for AVX2, 4 for AVX512
        for (size_t j = 0; j < sizeof(mask); j++)
        {
            // Also, we could add a branch to sparse_nibble_offset increment to skip in cases where bytemask is zero,
            // and save a store_si above in that case
            uint8_t bytemask = (mask >> (8 * j)) & 0xFF;
            const auto& entry = sparse_affine_table.entry[bytemask];
            auto indicies = _mm_load_si128(reinterpret_cast<const __m128i*>(entry.indicies.data()));
            indicies = _mm_add_epi32(indicies, sparse_nibble_offset);
            assert(sparse_nibbles_size + entry.count <= sparse_nibbles.size());
            auto* sparse_nibbles_end = reinterpret_cast<__m128i*>(&sparse_nibbles[sparse_nibbles_size]);
            _mm_storeu_si128(sparse_nibbles_end, indicies);
            sparse_nibbles_size += entry.count;
            sparse_nibble_offset = _mm_add_epi32(sparse_nibble_offset, sparse_nibble_offset_adj);
        }
    }
    for (size_t i = 0; i < FT_SIZE / 2; i += stride * 4)
    {
        auto nstm_vec1 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i]));
        auto nstm_vec2 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + stride]));
        auto nstm_vec3 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + stride * 2]));
        auto nstm_vec4 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + stride * 3]));
        nstm_vec1 = SIMD::max_epi16(zero, nstm_vec1);
        nstm_vec2 = SIMD::max_epi16(zero, nstm_vec2);
        nstm_vec3 = SIMD::max_epi16(zero, nstm_vec3);
        nstm_vec4 = SIMD::max_epi16(zero, nstm_vec4);
        nstm_vec1 = SIMD::slli_epi16(nstm_vec1, 7);
        nstm_vec2 = SIMD::slli_epi16(nstm_vec2, 7);
        nstm_vec3 = SIMD::slli_epi16(nstm_vec3, 7);
        nstm_vec4 = SIMD::slli_epi16(nstm_vec4, 7);
        auto nstm_vec5 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + FT_SIZE / 2]));
        auto nstm_vec6 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + FT_SIZE / 2 + stride]));
        auto nstm_vec7 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + FT_SIZE / 2 + stride * 2]));
        auto nstm_vec8 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + FT_SIZE / 2 + stride * 3]));
        nstm_vec1 = SIMD::mulhi_epi16(nstm_vec1, nstm_vec5);
        nstm_vec2 = SIMD::mulhi_epi16(nstm_vec2, nstm_vec6);
        nstm_vec3 = SIMD::mulhi_epi16(nstm_vec3, nstm_vec7);
        nstm_vec4 = SIMD::mulhi_epi16(nstm_vec4, nstm_vec8);
        nstm_vec1 = SIMD::packus_epi16(nstm_vec1, nstm_vec2);
        nstm_vec3 = SIMD::packus_epi16(nstm_vec3, nstm_vec4);
        SIMD::store_si(&output[i + FT_SIZE / 2], nstm_vec1);
        SIMD::store_si(&output[i + FT_SIZE / 2 + stride * 2], nstm_vec3);
        auto mask1 = SIMD::cmpgt_epi32_mask(nstm_vec1);
        auto mask2 = SIMD::cmpgt_epi32_mask(nstm_vec3);
#if defined(USE_AVX512)
        uint32_t mask = mask1 | (mask2 << 16);
#elif defined(USE_AVX2)
        uint16_t mask = mask1 | (mask2 << 8);
#else
        uint8_t mask = mask1 | (mask2 << 4);
#endif
        for (size_t j = 0; j < sizeof(mask); j++)
        {
            uint8_t bytemask = (mask >> (8 * j)) & 0xFF;
            const auto& entry = sparse_affine_table.entry[bytemask];
            auto indicies = _mm_load_si128(reinterpret_cast<const __m128i*>(entry.indicies.data()));
            indicies = _mm_add_epi32(indicies, sparse_nibble_offset);
            assert(sparse_nibbles_size + entry.count <= sparse_nibbles.size());
            auto* sparse_nibbles_end = reinterpret_cast<__m128i*>(&sparse_nibbles[sparse_nibbles_size]);
            _mm_storeu_si128(sparse_nibbles_end, indicies);
            sparse_nibbles_size += entry.count;
            sparse_nibble_offset = _mm_add_epi32(sparse_nibble_offset, sparse_nibble_offset_adj);
        }
    }
#else
    for (size_t i = 0; i < FT_SIZE / 2; i++)
    {
        int16_t a = std::clamp(stm[i], int16_t(0), FT_SCALE);
        int16_t b = std::clamp(stm[i + FT_SIZE / 2], int16_t(0), FT_SCALE);
        uint8_t mul = (a * (b << 7)) >> 16;
        output[i] = mul;
    }

    for (size_t i = 0; i < FT_SIZE / 2; i++)
    {
        int16_t a = std::clamp(nstm[i], int16_t(0), FT_SCALE);
        int16_t b = std::clamp(nstm[i + FT_SIZE / 2], int16_t(0), FT_SCALE);
        uint8_t mul = (a * (b << 7)) >> 16;
        output[i + FT_SIZE / 2] = mul;
    }
#endif
}

void L1_activation(const std::array<uint8_t, FT_SIZE>& ft_activation,
    const std::array<int8_t, FT_SIZE * L1_SIZE>& l1_weight, const std::array<int32_t, L1_SIZE>& l1_bias,
    [[maybe_unused]] const std::array<int16_t, FT_SIZE / 4> sparse_nibbles,
    [[maybe_unused]] const size_t sparse_nibbles_size, std::array<float, L1_SIZE>& output)
{
#ifdef NETWORK_SHUFFLE
    shuffle_network_data.report_ft_activations(ft_activation);
    block_count += FT_SIZE / 4;
    block_nnz += sparse_nibbles_size;
    neuron_count += FT_SIZE;
    neuron_nnz += std::count_if(ft_activation.begin(), ft_activation.end(), [](auto val) { return val > 0; });
    if (block_count % (FT_SIZE / 4 * 1024) == 0)
    {
        std::cout << "Average block NNZ: " << float(block_nnz) / float(block_count) * 100 << "%\n";
        std::cout << "Average neuron NNZ: " << float(neuron_nnz) / float(neuron_count) * 100 << "%\n";
    }
#endif

#if defined(SIMD_ENABLED)
    constexpr auto stride = SIMD::vec_size / sizeof(int32_t);
    static_assert(L1_SIZE % stride == 0);
    SIMD::veci output_reg[L1_SIZE / stride];

    for (size_t i = 0; i < L1_SIZE; i += stride)
    {
        output_reg[i / stride] = SIMD::load_si(&l1_bias[i]);
    }

    for (size_t i = 0; i < sparse_nibbles_size; i++)
    {
        const auto& nibble_idx = sparse_nibbles[i];
        assert(0 <= nibble_idx && nibble_idx <= (int)FT_SIZE / 4);
        const auto ft_nibble = *(reinterpret_cast<const int32_t*>(ft_activation.data()) + nibble_idx);
        const auto ft_vec = SIMD::set1_epi32(ft_nibble);
        static_assert(L1_SIZE % (stride) == 0);
        for (size_t j = 0; j < L1_SIZE; j += stride)
        {
            output_reg[j / stride] = SIMD::dpbusd_epi32(
                output_reg[j / stride], ft_vec, SIMD::load_si(&l1_weight[nibble_idx * (4 * L1_SIZE) + j * 4]));
        }
    }

    const auto zero = SIMD::setzero_si();
    const auto one = SIMD::set1_epi32(127.f * L1_SCALE);
    const auto one_reciprocal = SIMD::set1_ps(1.f / (127.f * L1_SCALE)); // 127 to match FT_activation adjustment

    for (size_t i = 0; i < L1_SIZE; i += stride)
    {
        output_reg[i / stride] = SIMD::max_epi32(zero, output_reg[i / stride]);
        output_reg[i / stride] = SIMD::min_epi32(one, output_reg[i / stride]);
        auto vec = SIMD::cvtepi32_ps(output_reg[i / stride]);
        vec = SIMD::mul_ps(vec, one_reciprocal);
        SIMD::store_ps(&output[i], vec);
    }
#else
    for (size_t i = 0; i < L1_SIZE; i++)
    {
        int32_t int_output = l1_bias[i];

        for (size_t j = 0; j < FT_SIZE; j++)
        {
            int_output += ft_activation[j] * l1_weight[i * FT_SIZE + j];
        }

        // 127 to match the FT_activation adjustment
        output[i] = std::clamp(int_output / float(127 * L1_SCALE), 0.f, 1.f);
    }
#endif
}

void L2_activation(const std::array<float, L1_SIZE>& l1_activation,
    const std::array<std::array<float, L2_SIZE>, L1_SIZE>& l2_weight, std::array<float, L2_SIZE>& output)
{
#if defined(SIMD_ENABLED)
    constexpr auto stride = SIMD::vec_size / sizeof(float);
    static_assert(L2_SIZE % stride == 0);
    SIMD::vecs l2_reg[L2_SIZE / stride];

    for (size_t i = 0; i < L2_SIZE; i += stride)
    {
        l2_reg[i / stride] = SIMD::load_ps(&output[i]);
    }

    // We would normally calculate a vector-matrix product by calculating the dot product of each row. In this case, the
    // number of outputs is small enough that it is more efficient to transpose the weights, and hold the accumulated
    // outputs in temporary registers as we go. It also makes the activation much simpler with a simple max/min_ps.
    for (size_t i = 0; i < L1_SIZE; i++)
    {
        auto input = SIMD::set1_ps(l1_activation[i]);
        for (size_t j = 0; j < L2_SIZE; j += stride)
        {
            l2_reg[j / stride] = SIMD::fmadd_ps(input, SIMD::load_ps(&l2_weight[i][j]), l2_reg[j / stride]);
        }
    }

    const auto zero = SIMD::setzero_ps();
    const auto one = SIMD::set1_ps(1.f);

    for (size_t i = 0; i < L2_SIZE; i += stride)
    {
        auto result = l2_reg[i / stride];
        result = SIMD::max_ps(result, zero);
        result = SIMD::min_ps(result, one);
        SIMD::store_ps(&output[i], result);
    }

#else
    std::array<float, L1_SIZE> l1_float;
    for (size_t i = 0; i < L1_SIZE; i++)
    {
        // 127 to match FT_activation adjustment
        l1_float[i] = float(l1_activation[i]) * (1.f / 127 / L1_SCALE);
    }

    for (size_t i = 0; i < L2_SIZE; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            output[i] += l1_float[j] * l2_weight[i][j];
        }

        output[i] = std::clamp(output[i], 0.f, 1.f);
    }
#endif
}

void L3_activation(
    const std::array<float, L2_SIZE>& l2_activation, const std::array<float, L2_SIZE>& l3_weight, float& output)
{
#if defined(SIMD_ENABLED)
    constexpr auto stride = SIMD::vec_size / sizeof(float);
    static_assert(L2_SIZE % stride == 0);

    auto result = SIMD::setzero_ps();
    for (size_t i = 0; i < L2_SIZE; i += stride)
    {
        auto vec = SIMD::load_ps(&l2_activation[i]);
        result = SIMD::fmadd_ps(vec, SIMD::load_ps(&l3_weight[i]), result);
    }

    output += SIMD::hsum_ps(result);
#else
    for (size_t i = 0; i < L2_SIZE; i++)
    {
        output += l2_activation[i] * l3_weight[i];
    }
#endif
}
}