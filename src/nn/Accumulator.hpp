#pragma once

#include "Network.h"
#include "simd/Definitions.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <immintrin.h>

namespace NN::Accumulator
{

template <size_t SIZE>
void add1(std::array<int16_t, SIZE>& a, const std::array<int16_t, SIZE>& b)
{
#if defined(USE_SSE4)
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % stride == 0);
    for (size_t i = 0; i < SIZE; i += stride)
    {
        auto a_vec = SIMD::load_si(&a[i]);
        auto b_vec = SIMD::load_si(&b[i]);
        a_vec = SIMD::add_epi16(a_vec, b_vec);
        SIMD::store_si(&a[i], a_vec);
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
#if defined(USE_SSE4)
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % stride == 0);
    for (size_t i = 0; i < SIZE; i += stride)
    {
        auto a_vec = SIMD::load_si(&a[i]);
        auto b_vec = SIMD::load_si(&b[i]);
        a_vec = SIMD::sub_epi16(a_vec, b_vec);
        SIMD::store_si(&a[i], a_vec);
    }
#else
    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
    {
        a[i] -= b[i];
    }
#endif
}

template <size_t SIZE>
void add1sub1(std::array<int16_t, SIZE>& a, const std::array<int16_t, SIZE>& b, const std::array<int16_t, SIZE>& c,
    const std::array<int16_t, SIZE>& d)
{
#if defined(USE_SSE4)
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % stride == 0);
    for (size_t i = 0; i < SIZE; i += stride)
    {
        auto b_vec = SIMD::load_si(&b[i]);
        auto c_vec = SIMD::load_si(&c[i]);
        auto d_vec = SIMD::load_si(&d[i]);
        b_vec = SIMD::add_epi16(b_vec, c_vec);
        b_vec = SIMD::sub_epi16(b_vec, d_vec);
        SIMD::store_si(&a[i], b_vec);
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
#if defined(USE_SSE4)
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % stride == 0);
    for (size_t i = 0; i < SIZE; i += stride)
    {
        auto b_vec = SIMD::load_si(&b[i]);
        auto c_vec = SIMD::load_si(&c[i]);
        auto d_vec = SIMD::load_si(&d[i]);
        auto e_vec = SIMD::load_si(&e[i]);
        b_vec = SIMD::add_epi16(b_vec, c_vec);
        b_vec = SIMD::sub_epi16(b_vec, d_vec);
        b_vec = SIMD::sub_epi16(b_vec, e_vec);
        SIMD::store_si(&a[i], b_vec);
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
#if defined(USE_SSE4)
    constexpr auto stride = SIMD::vec_size / sizeof(int16_t);
    static_assert(SIZE % stride == 0);
    for (size_t i = 0; i < SIZE; i += stride)
    {
        auto b_vec = SIMD::load_si(&b[i]);
        auto c_vec = SIMD::load_si(&c[i]);
        auto d_vec = SIMD::load_si(&d[i]);
        auto e_vec = SIMD::load_si(&e[i]);
        auto f_vec = SIMD::load_si(&f[i]);
        b_vec = SIMD::add_epi16(b_vec, c_vec);
        b_vec = SIMD::add_epi16(b_vec, d_vec);
        b_vec = SIMD::sub_epi16(b_vec, e_vec);
        b_vec = SIMD::sub_epi16(b_vec, f_vec);
        SIMD::store_si(&a[i], b_vec);
    }
#else
    for (size_t i = 0; i < SIZE; i++)
    {
        a[i] = b[i] + c[i] + d[i] - e[i] - f[i];
    }
#endif
}

}