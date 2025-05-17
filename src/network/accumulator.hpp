#pragma once

#include "simd/definitions.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <immintrin.h>

namespace NN
{

template <size_t SIZE>
void add1(std::array<int16_t, SIZE>& a, const std::array<int16_t, SIZE>& b)
{
#if defined(SIMD_ENABLED)
    // Manually unrolled and interleaved x2
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % (stride * 2) == 0);
    for (size_t i = 0; i < SIZE; i += stride * 2)
    {
        auto a_vec1 = SIMD::load_si(&a[i]);
        auto a_vec2 = SIMD::load_si(&a[i + stride]);
        a_vec1 = SIMD::add_epi16(a_vec1, SIMD::load_si(&b[i]));
        a_vec2 = SIMD::add_epi16(a_vec2, SIMD::load_si(&b[i + stride]));
        SIMD::store_si(&a[i], a_vec1);
        SIMD::store_si(&a[i + stride], a_vec2);
    }
#else
    for (size_t i = 0; i < SIZE; i++)
    {
        a[i] += b[i];
    }
#endif
}

template <size_t SIZE>
void sub1(std::array<int16_t, SIZE>& a, const std::array<int16_t, SIZE>& b)
{
#if defined(SIMD_ENABLED)
    // Manually unrolled and interleaved x2
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % (stride * 2) == 0);
    for (size_t i = 0; i < SIZE; i += stride * 2)
    {
        auto a_vec1 = SIMD::load_si(&a[i]);
        auto a_vec2 = SIMD::load_si(&a[i + stride]);
        a_vec1 = SIMD::sub_epi16(a_vec1, SIMD::load_si(&b[i]));
        a_vec2 = SIMD::sub_epi16(a_vec2, SIMD::load_si(&b[i + stride]));
        SIMD::store_si(&a[i], a_vec1);
        SIMD::store_si(&a[i + stride], a_vec2);
    }
#else
    for (size_t i = 0; i < SIZE; i++)
    {
        a[i] -= b[i];
    }
#endif
}

template <size_t SIZE>
void add1sub1(std::array<int16_t, SIZE>& a, const std::array<int16_t, SIZE>& b, const std::array<int16_t, SIZE>& c,
    const std::array<int16_t, SIZE>& d)
{
#if defined(SIMD_ENABLED)
    // Manually unrolled and interleaved x4
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % (stride * 4) == 0);
    for (size_t i = 0; i < SIZE; i += stride * 4)
    {
        auto b_vec1 = SIMD::load_si(&b[i]);
        auto b_vec2 = SIMD::load_si(&b[i + stride]);
        auto b_vec3 = SIMD::load_si(&b[i + stride * 2]);
        auto b_vec4 = SIMD::load_si(&b[i + stride * 3]);
        b_vec1 = SIMD::add_epi16(b_vec1, SIMD::load_si(&c[i]));
        b_vec2 = SIMD::add_epi16(b_vec2, SIMD::load_si(&c[i + stride]));
        b_vec3 = SIMD::add_epi16(b_vec3, SIMD::load_si(&c[i + stride * 2]));
        b_vec4 = SIMD::add_epi16(b_vec4, SIMD::load_si(&c[i + stride * 3]));
        b_vec1 = SIMD::sub_epi16(b_vec1, SIMD::load_si(&d[i]));
        b_vec2 = SIMD::sub_epi16(b_vec2, SIMD::load_si(&d[i + stride]));
        b_vec3 = SIMD::sub_epi16(b_vec3, SIMD::load_si(&d[i + stride * 2]));
        b_vec4 = SIMD::sub_epi16(b_vec4, SIMD::load_si(&d[i + stride * 3]));
        SIMD::store_si(&a[i], b_vec1);
        SIMD::store_si(&a[i + stride], b_vec2);
        SIMD::store_si(&a[i + stride * 2], b_vec3);
        SIMD::store_si(&a[i + stride * 3], b_vec4);
    }
#else
    for (size_t i = 0; i < SIZE; i++)
    {
        a[i] = b[i] + c[i] - d[i];
    }
#endif
}

template <size_t SIZE>
void add1sub2(std::array<int16_t, SIZE>& a, const std::array<int16_t, SIZE>& b, const std::array<int16_t, SIZE>& c,
    const std::array<int16_t, SIZE>& d, const std::array<int16_t, SIZE>& e)
{
#if defined(SIMD_ENABLED)
    // Manually unrolled and interleaved x4
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % (stride * 4) == 0);
    for (size_t i = 0; i < SIZE; i += stride * 4)
    {
        auto b_vec1 = SIMD::load_si(&b[i]);
        auto b_vec2 = SIMD::load_si(&b[i + stride]);
        auto b_vec3 = SIMD::load_si(&b[i + stride * 2]);
        auto b_vec4 = SIMD::load_si(&b[i + stride * 3]);
        b_vec1 = SIMD::add_epi16(b_vec1, SIMD::load_si(&c[i]));
        b_vec2 = SIMD::add_epi16(b_vec2, SIMD::load_si(&c[i + stride]));
        b_vec3 = SIMD::add_epi16(b_vec3, SIMD::load_si(&c[i + stride * 2]));
        b_vec4 = SIMD::add_epi16(b_vec4, SIMD::load_si(&c[i + stride * 3]));
        b_vec1 = SIMD::sub_epi16(b_vec1, SIMD::load_si(&d[i]));
        b_vec2 = SIMD::sub_epi16(b_vec2, SIMD::load_si(&d[i + stride]));
        b_vec3 = SIMD::sub_epi16(b_vec3, SIMD::load_si(&d[i + stride * 2]));
        b_vec4 = SIMD::sub_epi16(b_vec4, SIMD::load_si(&d[i + stride * 3]));
        b_vec1 = SIMD::sub_epi16(b_vec1, SIMD::load_si(&e[i]));
        b_vec2 = SIMD::sub_epi16(b_vec2, SIMD::load_si(&e[i + stride]));
        b_vec3 = SIMD::sub_epi16(b_vec3, SIMD::load_si(&e[i + stride * 2]));
        b_vec4 = SIMD::sub_epi16(b_vec4, SIMD::load_si(&e[i + stride * 3]));
        SIMD::store_si(&a[i], b_vec1);
        SIMD::store_si(&a[i + stride], b_vec2);
        SIMD::store_si(&a[i + stride * 2], b_vec3);
        SIMD::store_si(&a[i + stride * 3], b_vec4);
    }
#else
    for (size_t i = 0; i < SIZE; i++)
    {
        a[i] = b[i] + c[i] - d[i] - e[i];
    }
#endif
}

template <size_t SIZE>
void add2sub2(std::array<int16_t, SIZE>& a, const std::array<int16_t, SIZE>& b, const std::array<int16_t, SIZE>& c,
    const std::array<int16_t, SIZE>& d, const std::array<int16_t, SIZE>& e, const std::array<int16_t, SIZE>& f)
{
#if defined(SIMD_ENABLED)
    // Manually unrolled and interleaved x4
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % (stride * 4) == 0);
    for (size_t i = 0; i < SIZE; i += stride * 4)
    {
        auto b_vec1 = SIMD::load_si(&b[i]);
        auto b_vec2 = SIMD::load_si(&b[i + stride]);
        auto b_vec3 = SIMD::load_si(&b[i + stride * 2]);
        auto b_vec4 = SIMD::load_si(&b[i + stride * 3]);
        b_vec1 = SIMD::add_epi16(b_vec1, SIMD::load_si(&c[i]));
        b_vec2 = SIMD::add_epi16(b_vec2, SIMD::load_si(&c[i + stride]));
        b_vec3 = SIMD::add_epi16(b_vec3, SIMD::load_si(&c[i + stride * 2]));
        b_vec4 = SIMD::add_epi16(b_vec4, SIMD::load_si(&c[i + stride * 3]));
        b_vec1 = SIMD::add_epi16(b_vec1, SIMD::load_si(&d[i]));
        b_vec2 = SIMD::add_epi16(b_vec2, SIMD::load_si(&d[i + stride]));
        b_vec3 = SIMD::add_epi16(b_vec3, SIMD::load_si(&d[i + stride * 2]));
        b_vec4 = SIMD::add_epi16(b_vec4, SIMD::load_si(&d[i + stride * 3]));
        b_vec1 = SIMD::sub_epi16(b_vec1, SIMD::load_si(&e[i]));
        b_vec2 = SIMD::sub_epi16(b_vec2, SIMD::load_si(&e[i + stride]));
        b_vec3 = SIMD::sub_epi16(b_vec3, SIMD::load_si(&e[i + stride * 2]));
        b_vec4 = SIMD::sub_epi16(b_vec4, SIMD::load_si(&e[i + stride * 3]));
        b_vec1 = SIMD::sub_epi16(b_vec1, SIMD::load_si(&f[i]));
        b_vec2 = SIMD::sub_epi16(b_vec2, SIMD::load_si(&f[i + stride]));
        b_vec3 = SIMD::sub_epi16(b_vec3, SIMD::load_si(&f[i + stride * 2]));
        b_vec4 = SIMD::sub_epi16(b_vec4, SIMD::load_si(&f[i + stride * 3]));
        SIMD::store_si(&a[i], b_vec1);
        SIMD::store_si(&a[i + stride], b_vec2);
        SIMD::store_si(&a[i + stride * 2], b_vec3);
        SIMD::store_si(&a[i + stride * 3], b_vec4);
    }
#else
    for (size_t i = 0; i < SIZE; i++)
    {
        a[i] = b[i] + c[i] + d[i] - e[i] - f[i];
    }
#endif
}

}