#pragma once

#include "network/simd/intrinsics.hpp"

#if defined(USE_SSE4)
#include <immintrin.h>
#endif

#if defined(USE_NEON)
#include <arm_neon.h>
#endif

#include <cstdint>
#include <iostream>

namespace NN::SIMD
{

// https://stackoverflow.com/questions/60108658/fastest-method-to-calculate-sum-of-all-packed-32-bit-integers-using-avx512-or-av

#if defined(USE_SSE4)
inline int32_t hsum_i32(__m128i x)
{
    auto hi64 = _mm_unpackhi_epi64(x, x);
    auto sum64 = _mm_add_epi32(hi64, x);
    auto hi32 = _mm_shuffle_epi32(sum64, _MM_SHUFFLE(2, 3, 0, 1));
    auto sum32 = _mm_add_epi32(sum64, hi32);
    return _mm_cvtsi128_si32(sum32);
}
inline float hsum_f32(__m128 x)
{
    __m128 hi64 = _mm_movehl_ps(x, x);
    __m128 sum64 = _mm_add_ps(x, hi64);
    __m128 hi32 = _mm_shuffle_ps(sum64, sum64, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 sum32 = _mm_add_ps(sum64, hi32);
    return _mm_cvtss_f32(sum32);
}
#endif

#if defined(USE_AVX2)
inline int32_t hsum_i32(__m256i v)
{
    auto sum128 = _mm_add_epi32(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1));
    return hsum_i32(sum128);
}
inline float hsum_f32(__m256 v)
{
    __m128 sum128 = _mm_add_ps(_mm256_castps256_ps128(v), _mm256_extractf128_ps(v, 1));
    return hsum_f32(sum128);
}
#endif

#if defined(USE_AVX512)
inline int32_t hsum_i32(__m512i v)
{
    __m256i sum256 = _mm256_add_epi32(_mm512_castsi512_si256(v), _mm512_extracti64x4_epi64(v, 1));
    return hsum_i32(sum256);
}

inline float hsum_f32(__m512 v)
{
    __m256 sum256 = _mm256_add_ps(_mm512_castps512_ps256(v), _mm512_extractf32x8_ps(v, 1));
    return hsum_f32(sum256);
}
#endif

#if defined(USE_NEON)
inline float hsum_f32(float32x4_t v)
{
    // Can't use vaddvq_f32 here as it does not match SSE4 addition order exactly for bit-identical results
    float32x2_t hi64 = vget_high_f32(v);
    float32x2_t lo64 = vget_low_f32(v);
    float32x2_t sum64 = vadd_f32(lo64, hi64);
    return vget_lane_f32(sum64, 0) + vget_lane_f32(sum64, 1);
}
#endif

#if defined(SIMD_ENABLED)
template <class T, class R>
inline void Log(const R& value)
{
    const size_t n = sizeof(R) / sizeof(T);
    T buffer[n];
    SIMD::store((R*)buffer, value);
    for (size_t i = 0; i < n; i++)
        std::cout << (int)buffer[i] << " ";
    std::cout << std::endl;
}
#endif

}
