#pragma once

#include "network/simd/definitions.hpp"
#include "network/simd/utility.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <immintrin.h>
#include <iostream>

constexpr int16_t FT_SCALE = 255;
constexpr int16_t L1_SCALE = 64;
constexpr double SCALE_FACTOR = 160;

namespace NN
{

void FT_activation(const std::array<int16_t, FT_SIZE>& stm, const std::array<int16_t, FT_SIZE>& nstm,
    std::array<uint8_t, FT_SIZE * 2>& output)
{
#if defined(SIMD_ENABLED)
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(FT_SIZE % (stride * 2) == 0);
    const auto zero = SIMD::setzero_si();
    const auto one = SIMD::set1_epi16(FT_SCALE);
    for (size_t i = 0; i < FT_SIZE; i += stride * 2)
    {
        auto stm_vec1 = SIMD::min_epi16(one, SIMD::load_si(&stm[i]));
        auto stm_vec2 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + stride]));
        stm_vec1 = SIMD::max_epi16(zero, stm_vec1);
        stm_vec2 = SIMD::max_epi16(zero, stm_vec2);

        // Idea from Alexandria, we want to calculate the screlu using int16 without overflowing. In order to achieve
        // this we use mulhi_epi16 to calculate a temporary int32 product, before extracting the high 16 bits. With an
        // appropriate initial left shift, 255 * 255 * 2^7 / 2^16 = 127.00 we can calculate the screlu and then get a
        // value that fits into a int8 for the dense layer. This value when multiplied by the weights will give a result
        // of 127.00 * 64 * 1.98 = 16093, hence the maddubs_epi16 will not overflow.

        auto p1 = SIMD::slli_epi16(stm_vec1, 7);
        auto p2 = SIMD::slli_epi16(stm_vec2, 7);
        p1 = SIMD::mulhi_epi16(p1, stm_vec1);
        p2 = SIMD::mulhi_epi16(p2, stm_vec2);

        // We permute the weights at startup to match the packus.
        p1 = SIMD::packus_epi16(p1, p2);
        SIMD::store_si(&output[i], p1);
    }
    for (size_t i = 0; i < FT_SIZE; i += stride * 2)
    {
        auto nstm_vec1 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i]));
        auto nstm_vec2 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + stride]));
        nstm_vec1 = SIMD::max_epi16(zero, nstm_vec1);
        nstm_vec2 = SIMD::max_epi16(zero, nstm_vec2);
        auto p1 = SIMD::slli_epi16(nstm_vec1, 7);
        auto p2 = SIMD::slli_epi16(nstm_vec2, 7);
        p1 = SIMD::mulhi_epi16(p1, nstm_vec1);
        p2 = SIMD::mulhi_epi16(p2, nstm_vec2);
        p1 = SIMD::packus_epi16(p1, p2);
        SIMD::store_si(&output[i + FT_SIZE], p1);
    }
#else
    for (size_t i = 0; i < FT_SIZE; i++)
    {
        int16_t crelu = std::clamp(stm[i], int16_t(0), FT_SCALE);
        uint8_t screlu = (crelu * (crelu << 7)) >> 16;
        output[i] = screlu;
    }

    for (size_t i = 0; i < FT_SIZE; i++)
    {
        int16_t crelu = std::clamp(nstm[i], int16_t(0), FT_SCALE);
        uint8_t screlu = (crelu * (crelu << 7)) >> 16;
        output[i + FT_SIZE] = screlu;
    }
#endif
}

void L1_activation(const std::array<uint8_t, FT_SIZE * 2>& ft_activation,
    const std::array<std::array<int8_t, FT_SIZE * 2>, L1_SIZE>& l1_weight, std::array<int32_t, L1_SIZE>& output)
{
#if defined(SIMD_ENABLED)
    for (size_t i = 0; i < L1_SIZE; i++)
    {
        constexpr auto stride = SIMD::vec_size / sizeof(int8_t);
        static_assert((FT_SIZE * 2) % stride == 0);
        const auto madd_helper = SIMD::set1_epi16(1);
        auto r1 = SIMD::setzero_si();
        for (size_t j = 0; j < FT_SIZE * 2; j += stride)
        {
            // dpbusd_epi32 would be nice to use here (VNNI), and would also allow us to double the values in the
            // temporary int16 without overflow?
            auto vec1 = SIMD::load_si(&ft_activation[j]);
            vec1 = SIMD::maddubs_epi16(vec1, SIMD::load_si(&l1_weight[i][j]));
            vec1 = SIMD::madd_epi16(vec1, madd_helper);
            r1 = SIMD::add_epi32(r1, vec1);
        }
        output[i] += SIMD::hsum_epi32(r1);
        output[i] = std::clamp(output[i], int32_t(0), int32_t(127 * L1_SCALE));
    }
#else
    for (size_t i = 0; i < L1_SIZE; i++)
    {
        for (size_t j = 0; j < FT_SIZE * 2; j++)
        {
            output[i] += ft_activation[j] * l1_weight[i][j];
        }

        // 127 to match the FT_activation adjustment
        output[i] = std::clamp(output[i], int32_t(0), int32_t(127 * L1_SCALE));
    }
#endif
}

}