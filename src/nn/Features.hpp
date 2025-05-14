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

template <typename T_out, typename T_in, size_t SIZE>
void DotProductSCReLU(const std::array<T_in, SIZE>& stm, const std::array<T_in, SIZE>& nstm,
    const std::array<T_in, SIZE * 2>& weights, T_out& output)
{
#if defined(SIMD_ENABLED)
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % stride == 0);
    const auto zero = _mm256_setzero_si256();
    const auto one = _mm256_set1_epi16(L1_SCALE);
    auto result = zero;
    for (size_t i = 0; i < SIZE; i += stride)
    {
        auto stm_vec = SIMD::load_si(&stm[i]);
        auto weights_vec = SIMD::load_si(&weights[i]);
        stm_vec = SIMD::max_epi16(zero, SIMD::min_epi16(stm_vec, one));
        auto partial = SIMD::mullo_epi16(stm_vec, weights_vec);
        partial = SIMD::madd_epi16(partial, stm_vec);
        result = SIMD::add_epi32(result, partial);
    }
    for (size_t i = 0; i < SIZE; i += stride)
    {
        auto nstm_vec = SIMD::load_si(&nstm[i]);
        auto weights_vec = SIMD::load_si(&weights[i + SIZE]);
        nstm_vec = SIMD::max_epi16(zero, SIMD::min_epi16(nstm_vec, one));
        auto partial = SIMD::mullo_epi16(nstm_vec, weights_vec);
        partial = SIMD::madd_epi16(partial, nstm_vec);
        result = SIMD::add_epi32(result, partial);
    }
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