#pragma once

#include "network/simd/define.hpp"
#include "network/simd/intrinsics.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

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
        auto a_vec1 = SIMD::load(&a[i]);
        auto a_vec2 = SIMD::load(&a[i + stride]);
        a_vec1 = SIMD::add_i16(a_vec1, SIMD::load(&b[i]));
        a_vec2 = SIMD::add_i16(a_vec2, SIMD::load(&b[i + stride]));
        SIMD::store(&a[i], a_vec1);
        SIMD::store(&a[i + stride], a_vec2);
    }
#else
    for (size_t i = 0; i < SIZE; i++)
    {
        a[i] += b[i];
    }
#endif
}

template <size_t SIZE>
void add1(std::array<int16_t, SIZE>& a, const std::array<int8_t, SIZE>& b)
{
#if defined(SIMD_ENABLED)
    // Each iteration: load stride int8 values, widen to int16, add to accumulator
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % (stride * 2) == 0);
    for (size_t i = 0; i < SIZE; i += stride * 2)
    {
        auto a_vec1 = SIMD::load(&a[i]);
        auto a_vec2 = SIMD::load(&a[i + stride]);
        a_vec1 = SIMD::add_i16(a_vec1, SIMD::cvt_i8_to_i16(&b[i]));
        a_vec2 = SIMD::add_i16(a_vec2, SIMD::cvt_i8_to_i16(&b[i + stride]));
        SIMD::store(&a[i], a_vec1);
        SIMD::store(&a[i + stride], a_vec2);
    }
#else
    for (size_t i = 0; i < SIZE; i++)
    {
        a[i] += static_cast<int16_t>(b[i]);
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
        auto a_vec1 = SIMD::load(&a[i]);
        auto a_vec2 = SIMD::load(&a[i + stride]);
        a_vec1 = SIMD::sub_i16(a_vec1, SIMD::load(&b[i]));
        a_vec2 = SIMD::sub_i16(a_vec2, SIMD::load(&b[i + stride]));
        SIMD::store(&a[i], a_vec1);
        SIMD::store(&a[i + stride], a_vec2);
    }
#else
    for (size_t i = 0; i < SIZE; i++)
    {
        a[i] -= b[i];
    }
#endif
}

template <size_t SIZE>
void sub1(std::array<int16_t, SIZE>& a, const std::array<int8_t, SIZE>& b)
{
#if defined(SIMD_ENABLED)
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % (stride * 2) == 0);
    for (size_t i = 0; i < SIZE; i += stride * 2)
    {
        auto a_vec1 = SIMD::load(&a[i]);
        auto a_vec2 = SIMD::load(&a[i + stride]);
        a_vec1 = SIMD::sub_i16(a_vec1, SIMD::cvt_i8_to_i16(&b[i]));
        a_vec2 = SIMD::sub_i16(a_vec2, SIMD::cvt_i8_to_i16(&b[i + stride]));
        SIMD::store(&a[i], a_vec1);
        SIMD::store(&a[i + stride], a_vec2);
    }
#else
    for (size_t i = 0; i < SIZE; i++)
    {
        a[i] -= static_cast<int16_t>(b[i]);
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
        auto b_vec1 = SIMD::load(&b[i]);
        auto b_vec2 = SIMD::load(&b[i + stride]);
        auto b_vec3 = SIMD::load(&b[i + stride * 2]);
        auto b_vec4 = SIMD::load(&b[i + stride * 3]);
        b_vec1 = SIMD::add_i16(b_vec1, SIMD::load(&c[i]));
        b_vec2 = SIMD::add_i16(b_vec2, SIMD::load(&c[i + stride]));
        b_vec3 = SIMD::add_i16(b_vec3, SIMD::load(&c[i + stride * 2]));
        b_vec4 = SIMD::add_i16(b_vec4, SIMD::load(&c[i + stride * 3]));
        b_vec1 = SIMD::sub_i16(b_vec1, SIMD::load(&d[i]));
        b_vec2 = SIMD::sub_i16(b_vec2, SIMD::load(&d[i + stride]));
        b_vec3 = SIMD::sub_i16(b_vec3, SIMD::load(&d[i + stride * 2]));
        b_vec4 = SIMD::sub_i16(b_vec4, SIMD::load(&d[i + stride * 3]));
        SIMD::store(&a[i], b_vec1);
        SIMD::store(&a[i + stride], b_vec2);
        SIMD::store(&a[i + stride * 2], b_vec3);
        SIMD::store(&a[i + stride * 3], b_vec4);
    }
#else
    for (size_t i = 0; i < SIZE; i++)
    {
        a[i] = b[i] + c[i] - d[i];
    }
#endif
}

template <size_t SIZE>
void add1sub1(std::array<int16_t, SIZE>& a, const std::array<int16_t, SIZE>& b, const std::array<int8_t, SIZE>& c,
    const std::array<int8_t, SIZE>& d)
{
#if defined(SIMD_ENABLED)
    // Manually unrolled and interleaved x4
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % (stride * 4) == 0);
    for (size_t i = 0; i < SIZE; i += stride * 4)
    {
        auto b_vec1 = SIMD::load(&b[i]);
        auto b_vec2 = SIMD::load(&b[i + stride]);
        auto b_vec3 = SIMD::load(&b[i + stride * 2]);
        auto b_vec4 = SIMD::load(&b[i + stride * 3]);
        b_vec1 = SIMD::add_i16(b_vec1, SIMD::cvt_i8_to_i16(&c[i]));
        b_vec2 = SIMD::add_i16(b_vec2, SIMD::cvt_i8_to_i16(&c[i + stride]));
        b_vec3 = SIMD::add_i16(b_vec3, SIMD::cvt_i8_to_i16(&c[i + stride * 2]));
        b_vec4 = SIMD::add_i16(b_vec4, SIMD::cvt_i8_to_i16(&c[i + stride * 3]));
        b_vec1 = SIMD::sub_i16(b_vec1, SIMD::cvt_i8_to_i16(&d[i]));
        b_vec2 = SIMD::sub_i16(b_vec2, SIMD::cvt_i8_to_i16(&d[i + stride]));
        b_vec3 = SIMD::sub_i16(b_vec3, SIMD::cvt_i8_to_i16(&d[i + stride * 2]));
        b_vec4 = SIMD::sub_i16(b_vec4, SIMD::cvt_i8_to_i16(&d[i + stride * 3]));
        SIMD::store(&a[i], b_vec1);
        SIMD::store(&a[i + stride], b_vec2);
        SIMD::store(&a[i + stride * 2], b_vec3);
        SIMD::store(&a[i + stride * 3], b_vec4);
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
        auto b_vec1 = SIMD::load(&b[i]);
        auto b_vec2 = SIMD::load(&b[i + stride]);
        auto b_vec3 = SIMD::load(&b[i + stride * 2]);
        auto b_vec4 = SIMD::load(&b[i + stride * 3]);
        b_vec1 = SIMD::add_i16(b_vec1, SIMD::load(&c[i]));
        b_vec2 = SIMD::add_i16(b_vec2, SIMD::load(&c[i + stride]));
        b_vec3 = SIMD::add_i16(b_vec3, SIMD::load(&c[i + stride * 2]));
        b_vec4 = SIMD::add_i16(b_vec4, SIMD::load(&c[i + stride * 3]));
        b_vec1 = SIMD::sub_i16(b_vec1, SIMD::load(&d[i]));
        b_vec2 = SIMD::sub_i16(b_vec2, SIMD::load(&d[i + stride]));
        b_vec3 = SIMD::sub_i16(b_vec3, SIMD::load(&d[i + stride * 2]));
        b_vec4 = SIMD::sub_i16(b_vec4, SIMD::load(&d[i + stride * 3]));
        b_vec1 = SIMD::sub_i16(b_vec1, SIMD::load(&e[i]));
        b_vec2 = SIMD::sub_i16(b_vec2, SIMD::load(&e[i + stride]));
        b_vec3 = SIMD::sub_i16(b_vec3, SIMD::load(&e[i + stride * 2]));
        b_vec4 = SIMD::sub_i16(b_vec4, SIMD::load(&e[i + stride * 3]));
        SIMD::store(&a[i], b_vec1);
        SIMD::store(&a[i + stride], b_vec2);
        SIMD::store(&a[i + stride * 2], b_vec3);
        SIMD::store(&a[i + stride * 3], b_vec4);
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
        auto b_vec1 = SIMD::load(&b[i]);
        auto b_vec2 = SIMD::load(&b[i + stride]);
        auto b_vec3 = SIMD::load(&b[i + stride * 2]);
        auto b_vec4 = SIMD::load(&b[i + stride * 3]);
        b_vec1 = SIMD::add_i16(b_vec1, SIMD::load(&c[i]));
        b_vec2 = SIMD::add_i16(b_vec2, SIMD::load(&c[i + stride]));
        b_vec3 = SIMD::add_i16(b_vec3, SIMD::load(&c[i + stride * 2]));
        b_vec4 = SIMD::add_i16(b_vec4, SIMD::load(&c[i + stride * 3]));
        b_vec1 = SIMD::add_i16(b_vec1, SIMD::load(&d[i]));
        b_vec2 = SIMD::add_i16(b_vec2, SIMD::load(&d[i + stride]));
        b_vec3 = SIMD::add_i16(b_vec3, SIMD::load(&d[i + stride * 2]));
        b_vec4 = SIMD::add_i16(b_vec4, SIMD::load(&d[i + stride * 3]));
        b_vec1 = SIMD::sub_i16(b_vec1, SIMD::load(&e[i]));
        b_vec2 = SIMD::sub_i16(b_vec2, SIMD::load(&e[i + stride]));
        b_vec3 = SIMD::sub_i16(b_vec3, SIMD::load(&e[i + stride * 2]));
        b_vec4 = SIMD::sub_i16(b_vec4, SIMD::load(&e[i + stride * 3]));
        b_vec1 = SIMD::sub_i16(b_vec1, SIMD::load(&f[i]));
        b_vec2 = SIMD::sub_i16(b_vec2, SIMD::load(&f[i + stride]));
        b_vec3 = SIMD::sub_i16(b_vec3, SIMD::load(&f[i + stride * 2]));
        b_vec4 = SIMD::sub_i16(b_vec4, SIMD::load(&f[i + stride * 3]));
        SIMD::store(&a[i], b_vec1);
        SIMD::store(&a[i + stride], b_vec2);
        SIMD::store(&a[i + stride * 2], b_vec3);
        SIMD::store(&a[i + stride * 3], b_vec4);
    }
#else
    for (size_t i = 0; i < SIZE; i++)
    {
        a[i] = b[i] + c[i] + d[i] - e[i] - f[i];
    }
#endif
}

}
