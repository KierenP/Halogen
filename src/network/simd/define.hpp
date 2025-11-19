#pragma once

#if defined(USE_SSE4)
#include <immintrin.h>
#endif

#if defined(USE_NEON)
#include <arm_neon.h>
#endif

#if defined(USE_SSE4) || defined(USE_NEON)
#define SIMD_ENABLED
#endif

#if defined SIMD_ENABLED

#include <cstdint>
#include <utility>

namespace NN::SIMD
{

#if defined(USE_AVX512)
using veci16 = __m512i;
using veci32 = __m512i;
using veci8 = __m512i;
using vecu8 = __m512i;
using vecf32 = __m512;
constexpr size_t vec_size = 64;

#elif defined(USE_AVX2)
using veci16 = __m256i;
using veci32 = __m256i;
using veci8 = __m256i;
using vecu8 = __m256i;
using vecf32 = __m256;
constexpr size_t vec_size = 32;

#elif defined(USE_SSE4)
using veci16 = __m128i;
using veci32 = __m128i;
using veci8 = __m128i;
using vecu8 = __m128i;
using vecf32 = __m128;
constexpr size_t vec_size = 16;

#elif defined(USE_NEON)
using veci16 = int16x8_t;
using veci32 = int32x4_t;
using veci8 = int8x16_t;
using vecu8 = uint8x16_t;
using vecf32 = float32x4_t;
constexpr size_t vec_size = 16;
#endif

template <typename T>
struct basic_type
{
    using type = decltype([] {
        if constexpr (std::is_same_v<T, veci16>)
        {
            return int16_t {};
        }
        else if constexpr (std::is_same_v<T, veci32>)
        {
            return int32_t {};
        }
        else if constexpr (std::is_same_v<T, veci8>)
        {
            return int8_t {};
        }
        else if constexpr (std::is_same_v<T, vecu8>)
        {
            return uint8_t {};
        }
        else if constexpr (std::is_same_v<T, vecf32>)
        {
            return float {};
        }
    }());
};

template <typename T>
using basic_type_t = typename basic_type<T>::type;

template <typename T>
struct vec_type
{
    using type = decltype([] {
        if constexpr (std::is_same_v<T, int16_t>)
        {
            return veci16 {};
        }
        else if constexpr (std::is_same_v<T, int32_t>)
        {
            return veci32 {};
        }
        else if constexpr (std::is_same_v<T, int8_t>)
        {
            return veci8 {};
        }
        else if constexpr (std::is_same_v<T, uint8_t>)
        {
            return vecu8 {};
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            return vecf32 {};
        }
    }());
};

template <typename T>
using vec_type_t = typename vec_type<T>::type;

}

#endif