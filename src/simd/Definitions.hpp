#pragma once

// SSE4 or higher is required for SIMD
#include <cstdint>
#if defined(USE_SSE4)
#define SIMD_ENABLED
#endif

#if defined SIMD_ENABLED

#include <immintrin.h>

namespace SIMD
{

#if defined(USE_AVX512)
using vec = __m512i;
constexpr size_t vec_size = 64;
#elif defined(USE_AVX2)
using vec = __m256i;
constexpr size_t vec_size = 32;
#elif defined(USE_SSE4)
using vec = __m128i;
constexpr size_t vec_size = 16;
#endif

inline vec load_si(const void* ptr)
{
#if defined(USE_AVX512)
    return _mm512_load_si512(ptr);
#elif defined(USE_AVX2)
    return _mm256_load_si256(reinterpret_cast<const vec*>(ptr));
#elif defined(USE_SSE4)
    return _mm_load_si128(reinterpret_cast<const vec*>(ptr));
#endif
}

inline void store_si(void* ptr, const vec& v)
{
#if defined(USE_AVX512)
    return _mm512_store_si512(ptr, v);
#elif defined(USE_AVX2)
    return _mm256_store_si256(reinterpret_cast<vec*>(ptr), v);
#elif defined(USE_SSE4)
    return _mm_store_si128(reinterpret_cast<vec*>(ptr), v);
#endif
}

inline vec add_epi16(const vec& a, const vec& b)
{
#if defined(USE_AVX512)
    return _mm512_add_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_add_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_add_epi16(a, b);
#endif
}

inline vec add_epi32(const vec& a, const vec& b)
{
#if defined(USE_AVX512)
    return _mm512_add_epi32(a, b);
#elif defined(USE_AVX2)
    return _mm256_add_epi32(a, b);
#elif defined(USE_SSE4)
    return _mm_add_epi32(a, b);
#endif
}

inline vec sub_epi16(const vec& a, const vec& b)
{
#if defined(USE_AVX512)
    return _mm512_sub_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_sub_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_sub_epi16(a, b);
#endif
}

inline vec mullo_epi16(const vec& a, const vec& b)
{
#if defined(USE_AVX512)
    return _mm512_mullo_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_mullo_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_mullo_epi16(a, b);
#endif
}

inline vec madd_epi16(const vec& a, const vec& b)
{
#if defined(USE_AVX512)
    return _mm512_madd_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_madd_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_madd_epi16(a, b);
#endif
}

inline vec maddubs_epi16(const vec& a, const vec& b)
{
#if defined(USE_AVX512)
    return _mm512_maddubs_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_maddubs_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_maddubs_epi16(a, b);
#endif
}

inline vec max_epi16(const vec& a, const vec& b)
{
#if defined(USE_AVX512)
    return _mm512_max_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_max_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_max_epi16(a, b);
#endif
}

inline vec max_epi32(const vec& a, const vec& b)
{
#if defined(USE_AVX512)
    return _mm512_max_epi32(a, b);
#elif defined(USE_AVX2)
    return _mm256_max_epi32(a, b);
#elif defined(USE_SSE4)
    return _mm_max_epi32(a, b);
#endif
}

inline vec min_epi16(const vec& a, const vec& b)
{
#if defined(USE_AVX512)
    return _mm512_min_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_min_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_min_epi16(a, b);
#endif
}

inline vec min_epi32(const vec& a, const vec& b)
{
#if defined(USE_AVX512)
    return _mm512_min_epi32(a, b);
#elif defined(USE_AVX2)
    return _mm256_min_epi32(a, b);
#elif defined(USE_SSE4)
    return _mm_min_epi32(a, b);
#endif
}

inline vec setzero_si()
{
#if defined(USE_AVX512)
    return _mm512_setzero_si512();
#elif defined(USE_AVX2)
    return _mm256_setzero_si256();
#elif defined(USE_SSE4)
    return _mm_setzero_si128();
#endif
}

inline vec set1_epi16(int16_t a)
{
#if defined(USE_AVX512)
    return _mm512_set1_epi16(a);
#elif defined(USE_AVX2)
    return _mm256_set1_epi16(a);
#elif defined(USE_SSE4)
    return _mm_set1_epi16(a);
#endif
}

inline vec set1_epi32(int32_t a)
{
#if defined(USE_AVX512)
    return _mm512_set1_epi32(a);
#elif defined(USE_AVX2)
    return _mm256_set1_epi32(a);
#elif defined(USE_SSE4)
    return _mm_set1_epi32(a);
#endif
}

inline vec slli_epi16(const vec& a, int imm)
{
#if defined(USE_AVX512)
    return _mm512_slli_epi16(a, imm);
#elif defined(USE_AVX2)
    return _mm256_slli_epi16(a, imm);
#elif defined(USE_SSE4)
    return _mm_slli_epi16(a, imm);
#endif
}

inline vec mulhi_epi16(const vec& a, const vec& b)
{
#if defined(USE_AVX512)
    return _mm512_mulhi_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_mulhi_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_mulhi_epi16(a, b);
#endif
}

inline vec packus_epi16(const vec& a, const vec& b)
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
inline vec shuffle_epi32(const vec& a)
{
#if defined(USE_AVX512)
    return _mm512_shuffle_epi32(a, imm);
#elif defined(USE_AVX2)
    return _mm256_shuffle_epi32(a, imm);
#elif defined(USE_SSE4)
    return _mm_shuffle_epi32(a, imm);
#endif
}

inline uint16_t cmpgt_epi32_mask(const vec& a)
{
#if defined(USE_AVX512)
    return _mm512_cmpgt_epi32_mask(a, _mm512_setzero_si512());
#elif defined(USE_AVX2)
    return _mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(a, _mm256_setzero_si256())));
#elif defined(USE_SSE4)
    return _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(a, _mm_setzero_si128())));
#endif
}

}

#endif