#pragma once

#include "network/simd/definitions.hpp"
#include "network/simd/utility.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <immintrin.h>
#include <iostream>

// These quantization factors are selected to fit within certain bounds to avoid overflow while being as large as
// possible. In particular, we must avoid the following:
//  - accumulator (int16_t) overflow: (255 * 2 * 1.98) * (32 + 1) = 33323.4
//      [note that the *2 appears because of the factoriser. I need to train a network with lower weights to account for
//      this]. But the risk of overflow is very rare]
//  - l1 activation overflow (int16_t): (127 * 64 * 1.98) * 2 = 32186.88
//      [When doing maddubs_epi16, we must be able to take two adjacent FT activations * l1 weights]
constexpr int16_t FT_SCALE = 255;
constexpr int16_t L1_SCALE = 64;
constexpr double SCALE_FACTOR = 160;

namespace NN
{

void FT_activation(const std::array<int16_t, FT_SIZE>& stm, const std::array<int16_t, FT_SIZE>& nstm,
    std::array<uint8_t, FT_SIZE>& output)
{
#if defined(SIMD_ENABLED)
    // manually unrolled and interleaved x2, 10 max registers in use
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert((FT_SIZE / 2) % (stride * 4) == 0);
    const auto zero = SIMD::setzero_si();
    const auto one = SIMD::set1_epi16(FT_SCALE);
    for (size_t i = 0; i < FT_SIZE / 2; i += stride * 4)
    {
        // Idea from Alexandria, we want to calculate the screlu using int16 without overflowing. In order to achieve
        // this we use mulhi_epi16 to calculate a temporary int32 product, before extracting the high 16 bits. With an
        // appropriate initial left shift, 255 * 255 * 2^7 / 2^16 = 127.00 we can calculate the screlu and then get a
        // value that fits into a int8 for the dense layer. This value when multiplied by the weights will give a result
        // of 127.00 * 64 * 1.98 = 16093, hence the maddubs_epi16 will not overflow.

        // Another clever trick from Alexandria is we can skip two max_epi16 calls (one on each pairwise mul). This is
        // because mulhi_epi16 preserves the sign of the multiplication, meaning that after the packus we get the
        // correct result in all cases. For this to work, we need to make sure the int32 won't overflow, with
        // (-255 * 2) * 1.98 * (32+1) * 255 * 2^7 = -1B we are safe.

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
    }
    for (size_t i = 0; i < FT_SIZE / 2; i += stride * 2)
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

        // We permute the weights at startup to match the packus.
        nstm_vec1 = SIMD::packus_epi16(nstm_vec1, nstm_vec2);
        nstm_vec3 = SIMD::packus_epi16(nstm_vec3, nstm_vec4);
        SIMD::store_si(&output[i + FT_SIZE / 2], nstm_vec1);
        SIMD::store_si(&output[i + FT_SIZE / 2 + stride * 2], nstm_vec3);
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
    const std::array<std::array<int8_t, FT_SIZE>, L1_SIZE>& l1_weight, std::array<int32_t, L1_SIZE>& output)
{
#if defined(SIMD_ENABLED)
    const auto madd_helper = SIMD::set1_epi16(1);
    for (size_t i = 0; i < L1_SIZE; i++)
    {
        // manually unrolled and interleaved x4, max 9 registers in use
        constexpr auto stride = SIMD::vec_size / sizeof(int8_t);
        static_assert(FT_SIZE % (stride * 4) == 0);
        auto r1 = SIMD::setzero_si();
        auto r2 = SIMD::setzero_si();
        auto r3 = SIMD::setzero_si();
        auto r4 = SIMD::setzero_si();
        for (size_t j = 0; j < FT_SIZE; j += stride * 4)
        {
            // dpbusd_epi32 would be nice to use here (VNNI), and would also allow us to double the values in the
            // temporary int16 without overflow?
            auto vec1 = SIMD::load_si(&ft_activation[j]);
            auto vec2 = SIMD::load_si(&ft_activation[j + stride]);
            auto vec3 = SIMD::load_si(&ft_activation[j + stride * 2]);
            auto vec4 = SIMD::load_si(&ft_activation[j + stride * 3]);
            vec1 = SIMD::maddubs_epi16(vec1, SIMD::load_si(&l1_weight[i][j]));
            vec2 = SIMD::maddubs_epi16(vec2, SIMD::load_si(&l1_weight[i][j + stride]));
            vec3 = SIMD::maddubs_epi16(vec3, SIMD::load_si(&l1_weight[i][j + stride * 2]));
            vec4 = SIMD::maddubs_epi16(vec4, SIMD::load_si(&l1_weight[i][j + stride * 3]));
            vec1 = SIMD::madd_epi16(vec1, madd_helper);
            vec2 = SIMD::madd_epi16(vec2, madd_helper);
            vec3 = SIMD::madd_epi16(vec3, madd_helper);
            vec4 = SIMD::madd_epi16(vec4, madd_helper);
            r1 = SIMD::add_epi32(r1, vec1);
            r2 = SIMD::add_epi32(r2, vec2);
            r3 = SIMD::add_epi32(r3, vec3);
            r4 = SIMD::add_epi32(r4, vec4);
        }
        r1 = SIMD::add_epi32(r1, r2);
        r3 = SIMD::add_epi32(r3, r4);
        r1 = SIMD::add_epi32(r1, r3);
        output[i] += SIMD::hsum_epi32(r1);
        output[i] = std::clamp(output[i], int32_t(0), int32_t(127 * L1_SCALE));
    }
#else
    for (size_t i = 0; i < L1_SIZE; i++)
    {
        for (size_t j = 0; j < FT_SIZE; j++)
        {
            output[i] += ft_activation[j] * l1_weight[i][j];
        }

        // 127 to match the FT_activation adjustment
        output[i] = std::clamp(output[i], int32_t(0), int32_t(127 * L1_SCALE));
    }
#endif
}

void L2_activation(const std::array<int32_t, L1_SIZE>& l1_activation,
    const std::array<std::array<float, L1_SIZE>, L2_SIZE>& l2_weight, std::array<float, L2_SIZE>& output)
{
    // TODO: SIMD
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
}

void L3_activation(
    const std::array<float, L2_SIZE>& l2_activation, const std::array<float, L2_SIZE>& l3_weight, float& output)
{
    // TODO: SIMD
    for (size_t i = 0; i < L2_SIZE; i++)
    {
        output += l2_activation[i] * l3_weight[i];
    }
}
}