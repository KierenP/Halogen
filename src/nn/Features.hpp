#pragma once

#include "Network.h"
#include "simd/Definitions.hpp"
#include "simd/Utility.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <immintrin.h>

namespace NN::Features
{

constexpr int16_t L1_SCALE = 255;

template <size_t SIZE>
void DotProductSCReLU(const std::array<int16_t, SIZE>& stm, const std::array<int16_t, SIZE>& nstm,
    const std::array<int16_t, SIZE>& weights, int32_t& output)
{
#if defined(SIMD_ENABLED)
    // Manually unrolled and interleaved x2, using max 10 registers
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % (stride * 2) == 0);
    const auto zero = SIMD::setzero_si();
    const auto one = SIMD::set1_epi16(L1_SCALE);
    auto r1 = zero;
    auto r2 = zero;
    for (size_t i = 0; i < SIZE / 2; i += stride * 2)
    {
        auto stm_vec1 = SIMD::min_epi16(one, SIMD::load_si(&stm[i]));
        auto stm_vec2 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + stride]));
        stm_vec1 = SIMD::max_epi16(zero, stm_vec1);
        stm_vec2 = SIMD::max_epi16(zero, stm_vec2);
        auto p1 = SIMD::mullo_epi16(stm_vec1, SIMD::load_si(&weights[i]));
        auto p2 = SIMD::mullo_epi16(stm_vec2, SIMD::load_si(&weights[i + stride]));
        auto stm_vec3 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + SIZE / 2]));
        auto stm_vec4 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + SIZE / 2 + stride]));
        stm_vec3 = SIMD::max_epi16(zero, stm_vec3);
        stm_vec4 = SIMD::max_epi16(zero, stm_vec4);
        p1 = SIMD::madd_epi16(p1, stm_vec3);
        p2 = SIMD::madd_epi16(p2, stm_vec4);
        r1 = SIMD::add_epi32(r1, p1);
        r2 = SIMD::add_epi32(r2, p2);
    }
    for (size_t i = 0; i < SIZE / 2; i += stride * 2)
    {
        auto nstm_vec1 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i]));
        auto nstm_vec2 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + stride]));
        nstm_vec1 = SIMD::max_epi16(zero, nstm_vec1);
        nstm_vec2 = SIMD::max_epi16(zero, nstm_vec2);
        auto p1 = SIMD::mullo_epi16(nstm_vec1, SIMD::load_si(&weights[i + SIZE / 2]));
        auto p2 = SIMD::mullo_epi16(nstm_vec2, SIMD::load_si(&weights[i + SIZE / 2 + stride]));
        auto nstm_vec3 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + SIZE / 2]));
        auto nstm_vec4 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + SIZE / 2 + stride]));
        nstm_vec3 = SIMD::max_epi16(zero, nstm_vec3);
        nstm_vec4 = SIMD::max_epi16(zero, nstm_vec4);
        p1 = SIMD::madd_epi16(p1, nstm_vec3);
        p2 = SIMD::madd_epi16(p2, nstm_vec4);
        r1 = SIMD::add_epi32(r1, p1);
        r2 = SIMD::add_epi32(r2, p2);
    }
    auto result = SIMD::add_epi32(r1, r2);
    output += SIMD::hsum_epi32(result);
#else
    // This uses the lizard-SCReLU trick: Given stm[i] < 255, and weights[i] < 126, we want to compute stm[i] * stm[i] *
    // weights[i] to do the SCReLU dot product. By first doing stm[i] * weights[i], the result fits within the int16_t
    // type. Then multiply by stm[i] and accumulate.

    for (size_t i = 0; i < SIZE; i++)
    {
        int16_t crelu = std::clamp(stm[i], int16_t(0), L1_SCALE);
        int16_t partial = crelu * weights[i];
        output += partial * crelu;
    }

    for (size_t i = 0; i < SIZE; i++)
    {
        int16_t crelu = std::clamp(nstm[i], int16_t(0), L1_SCALE);
        int16_t partial = crelu * weights[i + SIZE];
        output += partial * crelu;
    }
#endif
}

}