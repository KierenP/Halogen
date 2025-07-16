#pragma once

// SSE4 or higher is required for SIMD
#include <cmath>
#include <cstdint>
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

inline veci load_si(const void* ptr)
{
#if defined(USE_AVX512)
    return _mm512_load_si512(ptr);
#elif defined(USE_AVX2)
    return _mm256_load_si256(reinterpret_cast<const veci*>(ptr));
#elif defined(USE_SSE4)
    return _mm_load_si128(reinterpret_cast<const veci*>(ptr));
#endif
}

inline void store_si(void* ptr, const veci& v)
{
#if defined(USE_AVX512)
    return _mm512_store_si512(ptr, v);
#elif defined(USE_AVX2)
    return _mm256_store_si256(reinterpret_cast<veci*>(ptr), v);
#elif defined(USE_SSE4)
    return _mm_store_si128(reinterpret_cast<veci*>(ptr), v);
#endif
}

inline veci add_epi16(const veci& a, const veci& b)
{
#if defined(USE_AVX512)
    return _mm512_add_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_add_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_add_epi16(a, b);
#endif
}

inline veci add_epi32(const veci& a, const veci& b)
{
#if defined(USE_AVX512)
    return _mm512_add_epi32(a, b);
#elif defined(USE_AVX2)
    return _mm256_add_epi32(a, b);
#elif defined(USE_SSE4)
    return _mm_add_epi32(a, b);
#endif
}

inline veci sub_epi16(const veci& a, const veci& b)
{
#if defined(USE_AVX512)
    return _mm512_sub_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_sub_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_sub_epi16(a, b);
#endif
}

inline veci mullo_epi16(const veci& a, const veci& b)
{
#if defined(USE_AVX512)
    return _mm512_mullo_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_mullo_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_mullo_epi16(a, b);
#endif
}

inline veci madd_epi16(const veci& a, const veci& b)
{
#if defined(USE_AVX512)
    return _mm512_madd_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_madd_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_madd_epi16(a, b);
#endif
}

inline veci maddubs_epi16(const veci& a, const veci& b)
{
#if defined(USE_AVX512)
    return _mm512_maddubs_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_maddubs_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_maddubs_epi16(a, b);
#endif
}

inline veci max_epi16(const veci& a, const veci& b)
{
#if defined(USE_AVX512)
    return _mm512_max_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_max_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_max_epi16(a, b);
#endif
}

inline veci max_epi32(const veci& a, const veci& b)
{
#if defined(USE_AVX512)
    return _mm512_max_epi32(a, b);
#elif defined(USE_AVX2)
    return _mm256_max_epi32(a, b);
#elif defined(USE_SSE4)
    return _mm_max_epi32(a, b);
#endif
}

inline veci min_epi16(const veci& a, const veci& b)
{
#if defined(USE_AVX512)
    return _mm512_min_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_min_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_min_epi16(a, b);
#endif
}

inline veci min_epi32(const veci& a, const veci& b)
{
#if defined(USE_AVX512)
    return _mm512_min_epi32(a, b);
#elif defined(USE_AVX2)
    return _mm256_min_epi32(a, b);
#elif defined(USE_SSE4)
    return _mm_min_epi32(a, b);
#endif
}

inline veci setzero_si()
{
#if defined(USE_AVX512)
    return _mm512_setzero_si512();
#elif defined(USE_AVX2)
    return _mm256_setzero_si256();
#elif defined(USE_SSE4)
    return _mm_setzero_si128();
#endif
}

inline veci set1_epi16(int16_t a)
{
#if defined(USE_AVX512)
    return _mm512_set1_epi16(a);
#elif defined(USE_AVX2)
    return _mm256_set1_epi16(a);
#elif defined(USE_SSE4)
    return _mm_set1_epi16(a);
#endif
}

inline veci set1_epi32(int32_t a)
{
#if defined(USE_AVX512)
    return _mm512_set1_epi32(a);
#elif defined(USE_AVX2)
    return _mm256_set1_epi32(a);
#elif defined(USE_SSE4)
    return _mm_set1_epi32(a);
#endif
}

inline veci slli_epi16(const veci& a, int imm)
{
#if defined(USE_AVX512)
    return _mm512_slli_epi16(a, imm);
#elif defined(USE_AVX2)
    return _mm256_slli_epi16(a, imm);
#elif defined(USE_SSE4)
    return _mm_slli_epi16(a, imm);
#endif
}

inline veci mulhi_epi16(const veci& a, const veci& b)
{
#if defined(USE_AVX512)
    return _mm512_mulhi_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_mulhi_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_mulhi_epi16(a, b);
#endif
}

inline veci packus_epi16(const veci& a, const veci& b)
{
#if defined(USE_AVX512)
    return _mm512_packus_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_packus_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_packus_epi16(a, b);
#endif
}

template <int imm>
inline veci shuffle_epi32(const veci& a)
{
#if defined(USE_AVX512)
    return _mm512_shuffle_epi32(a, imm);
#elif defined(USE_AVX2)
    return _mm256_shuffle_epi32(a, imm);
#elif defined(USE_SSE4)
    return _mm_shuffle_epi32(a, imm);
#endif
}

inline uint16_t cmpgt_epi32_mask(const veci& a)
{
#if defined(USE_AVX512)
    return _mm512_cmpgt_epi32_mask(a, _mm512_setzero_si512());
#elif defined(USE_AVX2)
    return _mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(a, _mm256_setzero_si256())));
#elif defined(USE_SSE4)
    return _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(a, _mm_setzero_si128())));
#endif
}

inline veci dpbusd_epi32(const veci& source, const veci& a, const veci& b)
{
#if defined(USE_AVX512_VNNI_)
    return _mm512_dpbusd_epi32(source, a, b);
#elif defined(USE_AVX512)
    static const auto madd_helper = SIMD::set1_epi16(1);
    auto dot = _mm512_maddubs_epi16(a, b);
    dot = _mm512_madd_epi16(dot, madd_helper);
    return _mm512_add_epi32(source, dot);
#elif defined(USE_AVX2)
    static const auto madd_helper = SIMD::set1_epi16(1);
    auto dot = _mm256_maddubs_epi16(a, b);
    dot = _mm256_madd_epi16(dot, madd_helper);
    return _mm256_add_epi32(source, dot);
#elif defined(USE_SSE4)
    static const auto madd_helper = SIMD::set1_epi16(1);
    auto dot = _mm_maddubs_epi16(a, b);
    dot = _mm_madd_epi16(dot, madd_helper);
    return _mm_add_epi32(source, dot);
#endif
}

inline vecs cvtepi32_ps(const veci& a)
{
#if defined(USE_AVX512)
    return _mm512_cvtepi32_ps(a);
#elif defined(USE_AVX2)
    return _mm256_cvtepi32_ps(a);
#elif defined(USE_SSE4)
    return _mm_cvtepi32_ps(a);
#endif
}

inline vecs setzero_ps()
{
#if defined(USE_AVX512)
    return _mm512_setzero_ps();
#elif defined(USE_AVX2)
    return _mm256_setzero_ps();
#elif defined(USE_SSE4)
    return _mm_setzero_ps();
#endif
}

inline vecs set1_ps(float a)
{
#if defined(USE_AVX512)
    return _mm512_set1_ps(a);
#elif defined(USE_AVX2)
    return _mm256_set1_ps(a);
#elif defined(USE_SSE4)
    return _mm_set1_ps(a);
#endif
}

inline vecs mul_ps(const vecs& a, const vecs& b)
{
#if defined(USE_AVX512)
    return _mm512_mul_ps(a, b);
#elif defined(USE_AVX2)
    return _mm256_mul_ps(a, b);
#elif defined(USE_SSE4)
    return _mm_mul_ps(a, b);
#endif
}

inline void store_ps(float* ptr, const vecs& v)
{
#if defined(USE_AVX512)
    return _mm512_store_ps(ptr, v);
#elif defined(USE_AVX2)
    return _mm256_store_ps(ptr, v);
#elif defined(USE_SSE4)
    return _mm_store_ps(ptr, v);
#endif
}

inline vecs add_ps(const vecs& a, const vecs& b)
{
#if defined(USE_AVX512)
    return _mm512_add_ps(a, b);
#elif defined(USE_AVX2)
    return _mm256_add_ps(a, b);
#elif defined(USE_SSE4)
    return _mm_add_ps(a, b);
#endif
}

inline vecs load_ps(const float* ptr)
{
#if defined(USE_AVX512)
    return _mm512_load_ps(ptr);
#elif defined(USE_AVX2)
    return _mm256_load_ps(ptr);
#elif defined(USE_SSE4)
    return _mm_load_ps(ptr);
#endif
}

inline vecs max_ps(const vecs& a, const vecs& b)
{
#if defined(USE_AVX512)
    return _mm512_max_ps(a, b);
#elif defined(USE_AVX2)
    return _mm256_max_ps(a, b);
#elif defined(USE_SSE4)
    return _mm_max_ps(a, b);
#endif
}

inline vecs min_ps(const vecs& a, const vecs& b)
{
#if defined(USE_AVX512)
    return _mm512_min_ps(a, b);
#elif defined(USE_AVX2)
    return _mm256_min_ps(a, b);
#elif defined(USE_SSE4)
    return _mm_min_ps(a, b);
#endif
}

inline vecs fmadd_ps(const vecs& a, const vecs& b, const vecs& c)
{
#if defined(USE_AVX512)
    return _mm512_fmadd_ps(a, b, c);
#elif defined(USE_AVX2)
    return _mm256_fmadd_ps(a, b, c);
#elif defined(USE_AVX)
    return _mm_fmadd_ps(a, b, c);
#elif defined(USE_SSE4)
    return _mm_add_ps(c, _mm_mul_ps(a, b));
#endif
}

}

#endif