#pragma once

#if defined(USE_SSE4)
#define SIMD_ENABLED
#endif

#if defined SIMD_ENABLED

#include <immintrin.h>

namespace NN::SIMD
{

#if defined(USE_AVX512)
using veci = __m512i;
using vecs = __m512;
constexpr size_t vec_size = 64;
#elif defined(USE_AVX2)
using veci = __m256i;
using vecs = __m256;
constexpr size_t vec_size = 32;
#elif defined(USE_SSE4)
using veci = __m128i;
using vecs = __m128;
constexpr size_t vec_size = 16;
#endif

}

#endif