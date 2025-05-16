#pragma once

#include "network/simd/definitions.hpp"
#include "network/simd/utility.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <immintrin.h>

constexpr int16_t L1_SCALE = 255;
constexpr int16_t L2_SCALE = 64;
constexpr double SCALE_FACTOR = 160;

namespace NN
{

template <size_t SIZE>
void inference_output(const std::array<int16_t, SIZE>& stm, const std::array<int16_t, SIZE>& nstm,
    const std::array<int8_t, SIZE * 2>& weights, int32_t& output)
{
#if defined(SIMD_ENABLED)
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % (stride * 2) == 0);
    const auto zero = SIMD::setzero_si();
    const auto one = SIMD::set1_epi16(L1_SCALE);
    const auto madd_helper = SIMD::set1_epi16(1);
    auto r1 = zero;
    for (size_t i = 0; i < SIZE; i += stride * 2)
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

        // We permute the weights at startup to match the packus. dpbusd_epi32 would be nice to use here, and would also
        // allow us to use 2^8 above without risk of overflow?
        p1 = SIMD::packus_epi16(p1, p2);
        p1 = SIMD::maddubs_epi16(p1, SIMD::load_si(&weights[i]));
        p1 = SIMD::madd_epi16(p1, madd_helper);

        r1 = SIMD::add_epi32(r1, p1);
    }
    for (size_t i = 0; i < SIZE; i += stride * 2)
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
        p1 = SIMD::maddubs_epi16(p1, SIMD::load_si(&weights[i + SIZE]));
        p1 = SIMD::madd_epi16(p1, madd_helper);
        r1 = SIMD::add_epi32(r1, p1);
    }
    output += SIMD::hsum_epi32(r1);
#else
    // This uses the lizard-SCReLU trick: Given stm[i] < 255, and weights[i] < 126, we want to compute stm[i] * stm[i] *
    // weights[i] to do the SCReLU dot product. By first doing stm[i] * weights[i], the result fits within the int16_t
    // type. Then multiply by stm[i] and accumulate.

    for (size_t i = 0; i < SIZE; i++)
    {
        int16_t crelu = std::clamp(stm[i], int16_t(0), L1_SCALE);
        uint8_t screlu = (crelu * crelu) >> 8;
        output += screlu * int8_t(weights[i]);
    }

    for (size_t i = 0; i < SIZE; i++)
    {
        int16_t crelu = std::clamp(nstm[i], int16_t(0), L1_SCALE);
        uint8_t screlu = (crelu * crelu) >> 8;
        output += screlu * int8_t(weights[i + SIZE]);
    }
#endif
}

}