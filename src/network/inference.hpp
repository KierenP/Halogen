#pragma once

#include "bitboard/define.h"
#include "network/arch.hpp"
#include "network/simd/define.hpp"
#include "network/simd/intrinsics.hpp"
#include "network/simd/utility.hpp"
#include "tools/sparse_shuffle.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

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

alignas(64) static constexpr std::array<int16_t, 32> nibble_offset_table = []
{
    std::array<int16_t, 32> cache {};
    for (int16_t i = 0; i < 32; ++i)
    {
        cache[i] = i;
    }
    return cache;
}();

void FT_activation(const std::array<int16_t, FT_SIZE>& stm, const std::array<int16_t, FT_SIZE>& nstm,
    std::array<uint8_t, FT_SIZE>& output, [[maybe_unused]] std::array<int16_t, FT_SIZE / 4>& sparse_nibbles,
    [[maybe_unused]] size_t& sparse_nibbles_size)
{
#if defined(SIMD_ENABLED)
    // manually unrolled and interleaved x2, 13 max registers in use
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert((FT_SIZE / 2) % (stride * 4) == 0);
    const auto zero = SIMD::setzero<SIMD::veci16>();
    const auto one = SIMD::set_i16(FT_SCALE);
#if defined(USE_AVX512_VNNI)
    auto sparse_nibble_offset = SIMD::set_i16(0);
    const auto sparse_nibble_offset_adj = SIMD::set_i16(32);
#elif defined(USE_NEON)
    auto sparse_nibble_offset = SIMD::set_i16(0);
    const auto sparse_nibble_offset_adj = SIMD::set_i16(8);
#else
    auto sparse_nibble_offset = _mm_set1_epi16(0);
    const auto sparse_nibble_offset_adj = _mm_set1_epi16(8);
#endif
    for (size_t i = 0; i < FT_SIZE / 2; i += stride * 4)
    {
        // Idea from Alexandria, we want to calculate the screlu using int16 without overflowing. In order to
        // achieve this we use mulhi_epi16 to calculate a temporary int32 product, before extracting the high 16
        // bits. With an appropriate initial left shift, 255 * 255 * 2^7 / 2^16 = 127.00 we can calculate the screlu
        // and then get a value that fits into a int8 for the dense layer. This value when multiplied by the weights
        // will give a result of 127.00 * 64 * 1.98 = 16093, hence the dpbusd_epi32 will not overflow.

        // Another clever trick from Alexandria is we can skip two max_epi16 calls (one on each pairwise mul). This
        // is because mulhi_epi16 preserves the sign of the multiplication, meaning that after the packus we get the
        // correct result in all cases. For this to work, we need to make sure the int32 won't overflow, with
        // (-255) * 1.98 * (32+1) * 255 * 2^7 = -500M we are safe.

        auto stm_vec1 = SIMD::min_i16(one, SIMD::load(&stm[i]));
        auto stm_vec2 = SIMD::min_i16(one, SIMD::load(&stm[i + stride]));
        auto stm_vec3 = SIMD::min_i16(one, SIMD::load(&stm[i + stride * 2]));
        auto stm_vec4 = SIMD::min_i16(one, SIMD::load(&stm[i + stride * 3]));
        stm_vec1 = SIMD::max_i16(zero, stm_vec1);
        stm_vec2 = SIMD::max_i16(zero, stm_vec2);
        stm_vec3 = SIMD::max_i16(zero, stm_vec3);
        stm_vec4 = SIMD::max_i16(zero, stm_vec4);
        stm_vec1 = SIMD::slli_i16(stm_vec1, 7);
        stm_vec2 = SIMD::slli_i16(stm_vec2, 7);
        stm_vec3 = SIMD::slli_i16(stm_vec3, 7);
        stm_vec4 = SIMD::slli_i16(stm_vec4, 7);
        auto stm_vec5 = SIMD::min_i16(one, SIMD::load(&stm[i + FT_SIZE / 2]));
        auto stm_vec6 = SIMD::min_i16(one, SIMD::load(&stm[i + FT_SIZE / 2 + stride]));
        auto stm_vec7 = SIMD::min_i16(one, SIMD::load(&stm[i + FT_SIZE / 2 + stride * 2]));
        auto stm_vec8 = SIMD::min_i16(one, SIMD::load(&stm[i + FT_SIZE / 2 + stride * 3]));
        stm_vec1 = SIMD::mulhi_i16(stm_vec1, stm_vec5);
        stm_vec2 = SIMD::mulhi_i16(stm_vec2, stm_vec6);
        stm_vec3 = SIMD::mulhi_i16(stm_vec3, stm_vec7);
        stm_vec4 = SIMD::mulhi_i16(stm_vec4, stm_vec8);

        // We permute the weights at startup to match the packus.
        stm_vec1 = SIMD::packus_i16(stm_vec1, stm_vec2);
        stm_vec3 = SIMD::packus_i16(stm_vec3, stm_vec4);
        SIMD::store(&output[i], stm_vec1);
        SIMD::store(&output[i + stride * 2], stm_vec3);

        // Cache the nonzero nibbles for sparse affine l1
        auto mask1 = SIMD::cmpgt_i32_mask(stm_vec1);
        auto mask2 = SIMD::cmpgt_i32_mask(stm_vec3);
#if defined(USE_AVX512)
        uint32_t mask = mask1 | (mask2 << 16);
#elif defined(USE_AVX2)
        uint16_t mask = mask1 | (mask2 << 8);
#else
        uint8_t mask = mask1 | (mask2 << 4);
#endif

#if defined(USE_AVX512_VNNI)
        auto indicies = SIMD::load(nibble_offset_table.data());
        indicies = SIMD::add_i16(indicies, sparse_nibble_offset);
        _mm512_mask_compressstoreu_epi16(&sparse_nibbles[sparse_nibbles_size], mask, indicies);
        sparse_nibbles_size += std::popcount(mask);
        sparse_nibble_offset = SIMD::add_i32(sparse_nibble_offset, sparse_nibble_offset_adj);
#else
        // 1 byte for SSE/NEON, 2 for AVX2, 4 for AVX512
        for (size_t j = 0; j < sizeof(mask); j++)
        {
            uint8_t bytemask = (mask >> (8 * j)) & 0xFF;
            const auto& entry = sparse_affine_table.entry[bytemask];
#if defined(USE_NEON)
            auto indicies = vld1q_s16(entry.indicies.data());
            indicies = SIMD::add_i16(indicies, sparse_nibble_offset);
            assert(sparse_nibbles_size + entry.count <= sparse_nibbles.size());
            vst1q_s16(&sparse_nibbles[sparse_nibbles_size], indicies);
            sparse_nibbles_size += entry.count;
            sparse_nibble_offset = SIMD::add_i16(sparse_nibble_offset, sparse_nibble_offset_adj);
#else
            auto indicies = _mm_load_si128(reinterpret_cast<const __m128i*>(entry.indicies.data()));
            indicies = _mm_add_epi16(indicies, sparse_nibble_offset);
            assert(sparse_nibbles_size + entry.count <= sparse_nibbles.size());
            auto* sparse_nibbles_end = reinterpret_cast<__m128i*>(&sparse_nibbles[sparse_nibbles_size]);
            _mm_storeu_si128(sparse_nibbles_end, indicies);
            sparse_nibbles_size += entry.count;
            sparse_nibble_offset = _mm_add_epi16(sparse_nibble_offset, sparse_nibble_offset_adj);
#endif
        }
#endif
    }
    for (size_t i = 0; i < FT_SIZE / 2; i += stride * 4)
    {
        auto nstm_vec1 = SIMD::min_i16(one, SIMD::load(&nstm[i]));
        auto nstm_vec2 = SIMD::min_i16(one, SIMD::load(&nstm[i + stride]));
        auto nstm_vec3 = SIMD::min_i16(one, SIMD::load(&nstm[i + stride * 2]));
        auto nstm_vec4 = SIMD::min_i16(one, SIMD::load(&nstm[i + stride * 3]));
        nstm_vec1 = SIMD::max_i16(zero, nstm_vec1);
        nstm_vec2 = SIMD::max_i16(zero, nstm_vec2);
        nstm_vec3 = SIMD::max_i16(zero, nstm_vec3);
        nstm_vec4 = SIMD::max_i16(zero, nstm_vec4);
        nstm_vec1 = SIMD::slli_i16(nstm_vec1, 7);
        nstm_vec2 = SIMD::slli_i16(nstm_vec2, 7);
        nstm_vec3 = SIMD::slli_i16(nstm_vec3, 7);
        nstm_vec4 = SIMD::slli_i16(nstm_vec4, 7);
        auto nstm_vec5 = SIMD::min_i16(one, SIMD::load(&nstm[i + FT_SIZE / 2]));
        auto nstm_vec6 = SIMD::min_i16(one, SIMD::load(&nstm[i + FT_SIZE / 2 + stride]));
        auto nstm_vec7 = SIMD::min_i16(one, SIMD::load(&nstm[i + FT_SIZE / 2 + stride * 2]));
        auto nstm_vec8 = SIMD::min_i16(one, SIMD::load(&nstm[i + FT_SIZE / 2 + stride * 3]));
        nstm_vec1 = SIMD::mulhi_i16(nstm_vec1, nstm_vec5);
        nstm_vec2 = SIMD::mulhi_i16(nstm_vec2, nstm_vec6);
        nstm_vec3 = SIMD::mulhi_i16(nstm_vec3, nstm_vec7);
        nstm_vec4 = SIMD::mulhi_i16(nstm_vec4, nstm_vec8);
        nstm_vec1 = SIMD::packus_i16(nstm_vec1, nstm_vec2);
        nstm_vec3 = SIMD::packus_i16(nstm_vec3, nstm_vec4);
        SIMD::store(&output[i + FT_SIZE / 2], nstm_vec1);
        SIMD::store(&output[i + FT_SIZE / 2 + stride * 2], nstm_vec3);
        auto mask1 = SIMD::cmpgt_i32_mask(nstm_vec1);
        auto mask2 = SIMD::cmpgt_i32_mask(nstm_vec3);
#if defined(USE_AVX512)
        uint32_t mask = mask1 | (mask2 << 16);
#elif defined(USE_AVX2)
        uint16_t mask = mask1 | (mask2 << 8);
#else
        uint8_t mask = mask1 | (mask2 << 4);
#endif

#if defined(USE_AVX512_VNNI)
        auto indicies = SIMD::load(nibble_offset_table.data());
        indicies = SIMD::add_i16(indicies, sparse_nibble_offset);
        _mm512_mask_compressstoreu_epi16(&sparse_nibbles[sparse_nibbles_size], mask, indicies);
        sparse_nibbles_size += std::popcount(mask);
        sparse_nibble_offset = SIMD::add_i32(sparse_nibble_offset, sparse_nibble_offset_adj);
#else
        // 1 byte for SSE/NEON, 2 for AVX2, 4 for AVX512
        for (size_t j = 0; j < sizeof(mask); j++)
        {
            uint8_t bytemask = (mask >> (8 * j)) & 0xFF;
            const auto& entry = sparse_affine_table.entry[bytemask];
#if defined(USE_NEON)
            auto indicies = vld1q_s16(entry.indicies.data());
            indicies = SIMD::add_i16(indicies, sparse_nibble_offset);
            assert(sparse_nibbles_size + entry.count <= sparse_nibbles.size());
            vst1q_s16(&sparse_nibbles[sparse_nibbles_size], indicies);
            sparse_nibbles_size += entry.count;
            sparse_nibble_offset = SIMD::add_i16(sparse_nibble_offset, sparse_nibble_offset_adj);
#else
            auto indicies = _mm_load_si128(reinterpret_cast<const __m128i*>(entry.indicies.data()));
            indicies = _mm_add_epi16(indicies, sparse_nibble_offset);
            assert(sparse_nibbles_size + entry.count <= sparse_nibbles.size());
            auto* sparse_nibbles_end = reinterpret_cast<__m128i*>(&sparse_nibbles[sparse_nibbles_size]);
            _mm_storeu_si128(sparse_nibbles_end, indicies);
            sparse_nibbles_size += entry.count;
            sparse_nibble_offset = _mm_add_epi16(sparse_nibble_offset, sparse_nibble_offset_adj);
#endif
        }
#endif
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
    [[maybe_unused]] const std::array<int16_t, FT_SIZE / 4>& sparse_nibbles,
    [[maybe_unused]] const size_t sparse_nibbles_size, std::array<float, L1_SIZE * 2>& output)
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
    SIMD::veci32 output_reg[L1_SIZE / stride];

    for (size_t i = 0; i < L1_SIZE; i += stride)
    {
        output_reg[i / stride] = SIMD::load(&l1_bias[i]);
    }

    for (size_t i = 0; i < sparse_nibbles_size; i++)
    {
        const auto& nibble_idx = sparse_nibbles[i];
        assert(0 <= nibble_idx && nibble_idx <= (int)FT_SIZE / 4);
        const auto ft_nibble = *(reinterpret_cast<const int32_t*>(ft_activation.data()) + nibble_idx);
        const auto ft_vec = SIMD::set_i32(ft_nibble);
        for (size_t j = 0; j < L1_SIZE; j += stride)
        {
            output_reg[j / stride] = SIMD::dpbusd_i32(
                output_reg[j / stride], ft_vec, SIMD::load(&l1_weight[nibble_idx * (4 * L1_SIZE) + j * 4]));
        }
    }

    const auto zero = SIMD::setzero<SIMD::vecf32>();
    const auto one = SIMD::set_f32(1.f);
    const auto one_reciprocal = SIMD::set_f32(1.f / (127.f * L1_SCALE)); // 127 to match FT_activation adjustment

    for (size_t i = 0; i < L1_SIZE; i += stride)
    {
        auto crelu = SIMD::i32_to_f32(output_reg[i / stride]);
        crelu = SIMD::mul_f32(crelu, one_reciprocal);
        auto screlu = SIMD::mul_f32(crelu, crelu);
        crelu = SIMD::max_f32(zero, crelu);
        crelu = SIMD::min_f32(one, crelu);
        screlu = SIMD::min_f32(one, screlu);
        SIMD::store(&output[i], crelu);
        SIMD::store(&output[i + L1_SIZE], screlu);
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
        float float_output = (float)int_output / float(127 * L1_SCALE);
        output[i] = std::clamp(float_output, 0.f, 1.f);
        output[i + L1_SIZE] = std::clamp(float_output * float_output, 0.f, 1.f);
    }
#endif
}

void L2_activation(const std::array<float, L1_SIZE * 2>& l1_activation,
    const std::array<std::array<float, L2_SIZE>, L1_SIZE * 2>& l2_weight, std::array<float, L2_SIZE>& output)
{
#if defined(SIMD_ENABLED)
    constexpr auto stride = SIMD::vec_size / sizeof(float);
    static_assert(L2_SIZE % stride == 0);
    SIMD::vecf32 l2_reg[L2_SIZE / stride];

    for (size_t i = 0; i < L2_SIZE; i += stride)
    {
        l2_reg[i / stride] = SIMD::load(&output[i]);
    }

    // We would normally calculate a vector-matrix product by calculating the dot product of each row. In this case, the
    // number of outputs is small enough that it is more efficient to transpose the weights, and hold the accumulated
    // outputs in temporary registers as we go. It also makes the activation much simpler with a simple max/min_ps.
    for (size_t i = 0; i < L1_SIZE * 2; i++)
    {
        auto input = SIMD::set_f32(l1_activation[i]);
        for (size_t j = 0; j < L2_SIZE; j += stride)
        {
            l2_reg[j / stride] = SIMD::fmadd_f32(input, SIMD::load(&l2_weight[i][j]), l2_reg[j / stride]);
        }
    }

    const auto zero = SIMD::setzero<SIMD::vecf32>();
    const auto one = SIMD::set_f32(1.f);

    for (size_t i = 0; i < L2_SIZE; i += stride)
    {
        auto result = l2_reg[i / stride];
        result = SIMD::max_f32(result, zero);
        result = SIMD::min_f32(result, one);
        SIMD::store(&output[i], result);
    }

#else
    for (size_t i = 0; i < L1_SIZE * 2; i++)
    {
        for (size_t j = 0; j < L2_SIZE; j++)
        {
            output[j] = std::fma(l1_activation[i], l2_weight[i][j], output[j]);
        }
    }

    for (size_t i = 0; i < L2_SIZE; i++)
    {
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

    SIMD::vecf32 results[L2_SIZE / stride];

    for (size_t i = 0; i < L2_SIZE; i += stride)
    {
        results[i / stride] = SIMD::setzero<SIMD::vecf32>();
    }

    for (size_t i = 0; i < L2_SIZE; i += stride)
    {
        auto vec = SIMD::load(&l2_activation[i]);
        results[i / stride] = SIMD::mul_f32(vec, SIMD::load(&l3_weight[i]));
    }

    // To make Halogen produce a stable bench on different platforms, we must add the floats in exactly the same way. To
    // achieve this, we can carefully combine the results together in the following way:

#if defined(USE_AVX512)
    results[0] = SIMD::add_f32(results[0], results[1]);
    output += SIMD::hsum_f32(results[0]);
#elif defined(USE_AVX2)
    results[0] = SIMD::add_f32(results[0], results[2]);
    results[1] = SIMD::add_f32(results[1], results[3]);
    results[0] = SIMD::add_f32(results[0], results[1]);
    output += SIMD::hsum_f32(results[0]);
#elif defined(USE_SSE4)
    results[0] = SIMD::add_f32(results[0], results[4]);
    results[1] = SIMD::add_f32(results[1], results[5]);
    results[2] = SIMD::add_f32(results[2], results[6]);
    results[3] = SIMD::add_f32(results[3], results[7]);
    results[0] = SIMD::add_f32(results[0], results[2]);
    results[1] = SIMD::add_f32(results[1], results[3]);
    results[0] = SIMD::add_f32(results[0], results[1]);
    output += SIMD::hsum_f32(results[0]);
#else
    static_assert(false, "Unsupported SIMD vector size");
#endif

#else
    // this whole song and dance is to match the order the floats are added in the SIMD code above
    float results[L2_SIZE];
    for (size_t i = 0; i < L2_SIZE; i++)
    {
        results[i] = l2_activation[i] * l3_weight[i];
    }
    size_t size = L2_SIZE;
    while (size > 1)
    {
        for (size_t i = 0; i < size / 2; i++)
        {
            results[i] += results[i + size / 2];
        }
        size /= 2;
    }
    output += results[0];
#endif
}
}