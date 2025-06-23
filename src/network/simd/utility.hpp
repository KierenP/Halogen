#pragma once

#include <cstdint>
#include <immintrin.h>

namespace NN::SIMD
{

// https://stackoverflow.com/questions/60108658/fastest-method-to-calculate-sum-of-all-packed-32-bit-integers-using-avx512-or-av
//
// This produces slightly better assembly than _mm512_reduce_add_epi32

#if defined(USE_SSE4)
inline int32_t hsum_epi32(__m128i x)
{
    auto hi64 = _mm_unpackhi_epi64(x, x);
    auto sum64 = _mm_add_epi32(hi64, x);
    auto hi32 = _mm_shuffle_epi32(sum64, _MM_SHUFFLE(2, 3, 0, 1));
    auto sum32 = _mm_add_epi32(sum64, hi32);
    return _mm_cvtsi128_si32(sum32);
}
#endif

#if defined(USE_AVX2)
inline int32_t hsum_epi32(__m256i v)
{
    auto sum128 = _mm_add_epi32(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1));
    return hsum_epi32(sum128);
}
#endif

#if defined(USE_AVX512)
inline int32_t hsum_epi32(__m512i v)
{
    auto sum256 = _mm256_add_epi32(_mm512_castsi512_si256(v), _mm512_extracti64x4_epi64(v, 1));
    return hsum_epi32(sum256);
}
#endif

}