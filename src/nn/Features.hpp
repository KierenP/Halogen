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
    const std::array<int16_t, SIZE * 2>& weights, int32_t& output)
{
#if defined(SIMD_ENABLED)
    // Manually unrolled and interleaved x4, using max 14 registers
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % (stride * 4) == 0);
    const auto zero = SIMD::setzero_si();
    const auto one = SIMD::set1_epi16(L1_SCALE);
    auto r1 = zero;
    auto r2 = zero;
    auto r3 = zero;
    auto r4 = zero;
    for (size_t i = 0; i < SIZE; i += stride * 4)
    {
        auto stm_vec1 = SIMD::min_epi16(one, SIMD::load_si(&stm[i]));
        auto stm_vec2 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + stride]));
        auto stm_vec3 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + stride * 2]));
        auto stm_vec4 = SIMD::min_epi16(one, SIMD::load_si(&stm[i + stride * 3]));
        stm_vec1 = SIMD::max_epi16(zero, stm_vec1);
        stm_vec2 = SIMD::max_epi16(zero, stm_vec2);
        stm_vec3 = SIMD::max_epi16(zero, stm_vec3);
        stm_vec4 = SIMD::max_epi16(zero, stm_vec4);
        auto p1 = SIMD::mullo_epi16(stm_vec1, SIMD::load_si(&weights[i]));
        auto p2 = SIMD::mullo_epi16(stm_vec2, SIMD::load_si(&weights[i + stride]));
        auto p3 = SIMD::mullo_epi16(stm_vec3, SIMD::load_si(&weights[i + stride * 2]));
        auto p4 = SIMD::mullo_epi16(stm_vec4, SIMD::load_si(&weights[i + stride * 3]));
        p1 = SIMD::madd_epi16(p1, stm_vec1);
        p2 = SIMD::madd_epi16(p2, stm_vec2);
        p3 = SIMD::madd_epi16(p3, stm_vec3);
        p4 = SIMD::madd_epi16(p4, stm_vec4);
        r1 = SIMD::add_epi32(r1, p1);
        r2 = SIMD::add_epi32(r2, p2);
        r3 = SIMD::add_epi32(r3, p3);
        r4 = SIMD::add_epi32(r4, p4);
    }
    for (size_t i = 0; i < SIZE; i += stride * 4)
    {
        auto nstm_vec1 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i]));
        auto nstm_vec2 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + stride]));
        auto nstm_vec3 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + stride * 2]));
        auto nstm_vec4 = SIMD::min_epi16(one, SIMD::load_si(&nstm[i + stride * 3]));
        nstm_vec1 = SIMD::max_epi16(zero, nstm_vec1);
        nstm_vec2 = SIMD::max_epi16(zero, nstm_vec2);
        nstm_vec3 = SIMD::max_epi16(zero, nstm_vec3);
        nstm_vec4 = SIMD::max_epi16(zero, nstm_vec4);
        auto p1 = SIMD::mullo_epi16(nstm_vec1, SIMD::load_si(&weights[i + SIZE]));
        auto p2 = SIMD::mullo_epi16(nstm_vec2, SIMD::load_si(&weights[i + SIZE + stride]));
        auto p3 = SIMD::mullo_epi16(nstm_vec3, SIMD::load_si(&weights[i + SIZE + stride * 2]));
        auto p4 = SIMD::mullo_epi16(nstm_vec4, SIMD::load_si(&weights[i + SIZE + stride * 3]));
        p1 = SIMD::madd_epi16(p1, nstm_vec1);
        p2 = SIMD::madd_epi16(p2, nstm_vec2);
        p3 = SIMD::madd_epi16(p3, nstm_vec3);
        p4 = SIMD::madd_epi16(p4, nstm_vec4);
        r1 = SIMD::add_epi32(r1, p1);
        r2 = SIMD::add_epi32(r2, p2);
        r3 = SIMD::add_epi32(r3, p3);
        r4 = SIMD::add_epi32(r4, p4);
    }
    auto result = SIMD::add_epi32(r1, r2);
    result = SIMD::add_epi32(result, r3);
    result = SIMD::add_epi32(result, r4);
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