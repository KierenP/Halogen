#pragma once

#include "network/simd/define.hpp"

#include <cmath>
#include <cstdint>
#include <type_traits>

#if defined SIMD_ENABLED

namespace NN::SIMD
{

template <class>
constexpr bool dependent_false = false; // workaround before CWG2518/P2593R1

template <typename T>
inline vec_type_t<T> load(const T* ptr)
{
    if constexpr (std::is_integral_v<T>)
    {
#if defined(USE_AVX512)
        return _mm512_load_si512(ptr);
#elif defined(USE_AVX2)
        return _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
#elif defined(USE_SSE4)
        return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
#elif defined(USE_NEON)
        if constexpr (std::is_same_v<T, int8_t>)
            return vld1q_s8(ptr);
        else if constexpr (std::is_same_v<T, uint8_t>)
            return vld1q_u8(ptr);
        else if constexpr (std::is_same_v<T, int16_t>)
            return vld1q_s16(ptr);
        else if constexpr (std::is_same_v<T, int32_t>)
            return vld1q_s32(ptr);
#endif
    }
    else if constexpr (std::is_same_v<T, float>)
    {
#if defined(USE_AVX512)
        return _mm512_load_ps(ptr);
#elif defined(USE_AVX2)
        return _mm256_load_ps(ptr);
#elif defined(USE_SSE4)
        return _mm_load_ps(ptr);
#elif defined(USE_NEON)
        return vld1q_f32(ptr);
#endif
    }
    else
    {
        static_assert(dependent_false<T>, "Unsupported type for SIMD::load");
    }
}

template <typename T>
inline void store(T* ptr, const vec_type_t<T>& v)
{
    if constexpr (std::is_integral_v<T>)
    {
#if defined(USE_AVX512)
        _mm512_store_si512(ptr, v);
#elif defined(USE_AVX2)
        _mm256_store_si256(reinterpret_cast<__m256i*>(ptr), v);
#elif defined(USE_SSE4)
        _mm_store_si128(reinterpret_cast<__m128i*>(ptr), v);
#elif defined(USE_NEON)
        if constexpr (std::is_same_v<T, int8_t>)
            vst1q_s8(ptr, v);
        else if constexpr (std::is_same_v<T, uint8_t>)
            vst1q_u8(ptr, v);
        else if constexpr (std::is_same_v<T, int16_t>)
            vst1q_s16(ptr, v);
        else if constexpr (std::is_same_v<T, int32_t>)
            vst1q_s32(ptr, v);
#endif
    }
    else if constexpr (std::is_same_v<T, float>)
    {
#if defined(USE_AVX512)
        _mm512_store_ps(ptr, v);
#elif defined(USE_AVX2)
        _mm256_store_ps(ptr, v);
#elif defined(USE_SSE4)
        _mm_store_ps(ptr, v);
#elif defined(USE_NEON)
        vst1q_f32(ptr, v);
#endif
    }
    else
    {
        static_assert(dependent_false<T>, "Unsupported type for SIMD::store");
    }
}

template <typename T>
inline T setzero()
{
    if constexpr (std::is_integral_v<basic_type_t<T>>)
    {
#if defined(USE_AVX512)
        return _mm512_setzero_si512();
#elif defined(USE_AVX2)
        return _mm256_setzero_si256();
#elif defined(USE_SSE4)
        return _mm_setzero_si128();
#elif defined(USE_NEON)
        if constexpr (std::is_same_v<T, int16x8_t>)
            return vdupq_n_s16(0);
        else if constexpr (std::is_same_v<T, int32x4_t>)
            return vdupq_n_s32(0);
        else if constexpr (std::is_same_v<T, int8x16_t>)
            return vdupq_n_s8(0);
        else if constexpr (std::is_same_v<T, uint8x16_t>)
            return vdupq_n_u8(0);
#endif
    }
    else if constexpr (std::is_floating_point_v<basic_type_t<T>>)
    {
#if defined(USE_AVX512)
        return _mm512_setzero_ps();
#elif defined(USE_AVX2)
        return _mm256_setzero_ps();
#elif defined(USE_SSE4)
        return _mm_setzero_ps();
#elif defined(USE_NEON)
        return vdupq_n_f32(0.0f);
#endif
    }
    else
    {
        static_assert(dependent_false<T>, "Unsupported type for SIMD::setzero");
    }
}

inline vecf32 set_f32(float a)
{
#if defined(USE_AVX512)
    return _mm512_set1_ps(a);
#elif defined(USE_AVX2)
    return _mm256_set1_ps(a);
#elif defined(USE_SSE4)
    return _mm_set1_ps(a);
#elif defined(USE_NEON)
    return vdupq_n_f32(a);
#endif
}

inline veci16 set_i16(int16_t a)
{
#if defined(USE_AVX512)
    return _mm512_set1_epi16(a);
#elif defined(USE_AVX2)
    return _mm256_set1_epi16(a);
#elif defined(USE_SSE4)
    return _mm_set1_epi16(a);
#elif defined(USE_NEON)
    return vdupq_n_s16(a);
#endif
}

inline veci32 set_i32(int32_t a)
{
#if defined(USE_AVX512)
    return _mm512_set1_epi32(a);
#elif defined(USE_AVX2)
    return _mm256_set1_epi32(a);
#elif defined(USE_SSE4)
    return _mm_set1_epi32(a);
#elif defined(USE_NEON)
    return vdupq_n_s32(a);
#endif
}

inline vecu8 set_u8_from_u32(uint32_t a)
{
#if defined(USE_AVX512)
    return _mm512_set1_epi32(static_cast<int32_t>(a));
#elif defined(USE_AVX2)
    return _mm256_set1_epi32(static_cast<int32_t>(a));
#elif defined(USE_SSE4)
    return _mm_set1_epi32(static_cast<int32_t>(a));
#elif defined(USE_NEON)
    return vreinterpretq_u8_u32(vdupq_n_u32(a));
#endif
}

inline veci16 add_i16(const veci16& a, const veci16& b)
{
#if defined(USE_AVX512)
    return _mm512_add_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_add_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_add_epi16(a, b);
#elif defined(USE_NEON)
    return vaddq_s16(a, b);
#endif
}

inline veci32 add_i32(const veci32& a, const veci32& b)
{
#if defined(USE_AVX512)
    return _mm512_add_epi32(a, b);
#elif defined(USE_AVX2)
    return _mm256_add_epi32(a, b);
#elif defined(USE_SSE4)
    return _mm_add_epi32(a, b);
#elif defined(USE_NEON)
    return vaddq_s32(a, b);
#endif
}

inline veci16 sub_i16(const veci16& a, const veci16& b)
{
#if defined(USE_AVX512)
    return _mm512_sub_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_sub_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_sub_epi16(a, b);
#elif defined(USE_NEON)
    return vsubq_s16(a, b);
#endif
}

inline veci16 max_i16(const veci16& a, const veci16& b)
{
#if defined(USE_AVX512)
    return _mm512_max_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_max_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_max_epi16(a, b);
#elif defined(USE_NEON)
    return vmaxq_s16(a, b);
#endif
}

inline veci32 max_i32(const veci32& a, const veci32& b)
{
#if defined(USE_AVX512)
    return _mm512_max_epi32(a, b);
#elif defined(USE_AVX2)
    return _mm256_max_epi32(a, b);
#elif defined(USE_SSE4)
    return _mm_max_epi32(a, b);
#elif defined(USE_NEON)
    return vmaxq_s32(a, b);
#endif
}

inline veci16 min_i16(const veci16& a, const veci16& b)
{
#if defined(USE_AVX512)
    return _mm512_min_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_min_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_min_epi16(a, b);
#elif defined(USE_NEON)
    return vminq_s16(a, b);
#endif
}

inline veci32 min_i32(const veci32& a, const veci32& b)
{
#if defined(USE_AVX512)
    return _mm512_min_epi32(a, b);
#elif defined(USE_AVX2)
    return _mm256_min_epi32(a, b);
#elif defined(USE_SSE4)
    return _mm_min_epi32(a, b);
#elif defined(USE_NEON)
    return vminq_s32(a, b);
#endif
}

template <int imm>
inline veci16 lshift_i16(const veci16& a)
{
#if defined(USE_AVX512)
    return _mm512_slli_epi16(a, imm);
#elif defined(USE_AVX2)
    return _mm256_slli_epi16(a, imm);
#elif defined(USE_SSE4)
    return _mm_slli_epi16(a, imm);
#elif defined(USE_NEON)
    return vshlq_n_s16(a, imm);
#endif
}

inline veci16 mulhi_i16(const veci16& a, const veci16& b)
{
#if defined(USE_AVX512)
    return _mm512_mulhi_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_mulhi_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_mulhi_epi16(a, b);
#elif defined(USE_NEON)
    // TODO: consider using vmulq_s16 + vqrshrun_n_s16 to combine with packus
    return vqdmulhq_s16(a, b);
#endif
}

inline vecu8 pack_i16_to_u8(const veci16& a, const veci16& b)
{
#if defined(USE_AVX512)
    return _mm512_packus_epi16(a, b);
#elif defined(USE_AVX2)
    return _mm256_packus_epi16(a, b);
#elif defined(USE_SSE4)
    return _mm_packus_epi16(a, b);
#elif defined(USE_NEON)
    uint8x8_t low = vqmovun_s16(a);
    uint8x8_t high = vqmovun_s16(b);
    return vcombine_u8(low, high);
#endif
}

inline uint16_t cmpgt_i32_mask(const vecu8& a)
{
#if defined(USE_AVX512)
    // On x86, vecu8 is __m512i, just compare as int32
    return _mm512_cmpgt_epi32_mask(a, _mm512_setzero_si512());
#elif defined(USE_AVX2)
    return _mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(a, _mm256_setzero_si256())));
#elif defined(USE_SSE4)
    return _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(a, _mm_setzero_si128())));
#elif defined(USE_NEON)
    uint32x4_t a_u32 = vreinterpretq_u32_u8(a);
    uint32x4_t cmp = vcgtq_u32(a_u32, vdupq_n_u32(0));
    uint32_t mask = 0;
    mask |= (vgetq_lane_u32(cmp, 0) & 1) << 0;
    mask |= (vgetq_lane_u32(cmp, 1) & 1) << 1;
    mask |= (vgetq_lane_u32(cmp, 2) & 1) << 2;
    mask |= (vgetq_lane_u32(cmp, 3) & 1) << 3;
    return mask;
#endif
}

static const auto madd_helper = SIMD::set_i16(1);

inline veci32 dpbusd_i32(const veci32& source, const vecu8& a, const veci8& b)
{
#if defined(USE_AVX512_VNNI)
    return _mm512_dpbusd_epi32(source, a, b);
#elif defined(USE_AVX512)
    auto dot = _mm512_maddubs_epi16(a, b);
    dot = _mm512_madd_epi16(dot, madd_helper);
    return _mm512_add_epi32(source, dot);
#elif defined(USE_AVX2)
    auto dot = _mm256_maddubs_epi16(a, b);
    dot = _mm256_madd_epi16(dot, madd_helper);
    return _mm256_add_epi32(source, dot);
#elif defined(USE_SSE4)
    auto dot = _mm_maddubs_epi16(a, b);
    dot = _mm_madd_epi16(dot, madd_helper);
    return _mm_add_epi32(source, dot);
#elif defined(USE_NEON)
    // safe because a is [0, 127], so can be treated as signed without overflow
    int8x8_t a_low = vreinterpret_s8_u8(vget_low_u8(a));
    int8x8_t a_high = vreinterpret_s8_u8(vget_high_u8(a));
    int8x8_t b_low = vget_low_s8(b);
    int8x8_t b_high = vget_high_s8(b);

    // Multiply i8 -> i16, add adjacent pairs, then add adjacent pairs again accumulating into i32
    int16x8_t prod_low = vmull_s8(a_low, b_low);
    int16x8_t prod_high = vmull_s8(a_high, b_high);
    int16x8_t sum = vpaddq_s16(prod_low, prod_high);
    return vpadalq_s16(source, sum);
#endif
}

inline vecf32 i32_to_f32(const veci32& a)
{
#if defined(USE_AVX512)
    return _mm512_cvtepi32_ps(a);
#elif defined(USE_AVX2)
    return _mm256_cvtepi32_ps(a);
#elif defined(USE_SSE4)
    return _mm_cvtepi32_ps(a);
#elif defined(USE_NEON)
    return vcvtq_f32_s32(a);
#endif
}

inline vecf32 mul_f32(const vecf32& a, const vecf32& b)
{
#if defined(USE_AVX512)
    return _mm512_mul_ps(a, b);
#elif defined(USE_AVX2)
    return _mm256_mul_ps(a, b);
#elif defined(USE_SSE4)
    return _mm_mul_ps(a, b);
#elif defined(USE_NEON)
    return vmulq_f32(a, b);
#endif
}

inline vecf32 add_f32(const vecf32& a, const vecf32& b)
{
#if defined(USE_AVX512)
    return _mm512_add_ps(a, b);
#elif defined(USE_AVX2)
    return _mm256_add_ps(a, b);
#elif defined(USE_SSE4)
    return _mm_add_ps(a, b);
#elif defined(USE_NEON)
    return vaddq_f32(a, b);
#endif
}

inline vecf32 max_f32(const vecf32& a, const vecf32& b)
{
#if defined(USE_AVX512)
    return _mm512_max_ps(a, b);
#elif defined(USE_AVX2)
    return _mm256_max_ps(a, b);
#elif defined(USE_SSE4)
    return _mm_max_ps(a, b);
#elif defined(USE_NEON)
    return vmaxq_f32(a, b);
#endif
}

inline vecf32 min_f32(const vecf32& a, const vecf32& b)
{
#if defined(USE_AVX512)
    return _mm512_min_ps(a, b);
#elif defined(USE_AVX2)
    return _mm256_min_ps(a, b);
#elif defined(USE_SSE4)
    return _mm_min_ps(a, b);
#elif defined(USE_NEON)
    return vminq_f32(a, b);
#endif
}

inline vecf32 fmadd_f32(const vecf32& a, const vecf32& b, const vecf32& c)
{
#if defined(USE_AVX512)
    return _mm512_fmadd_ps(a, b, c);
#elif defined(USE_AVX2)
    return _mm256_fmadd_ps(a, b, c);
#elif defined(USE_AVX)
    return _mm_fmadd_ps(a, b, c);
#elif defined(USE_SSE4)
    return _mm_add_ps(c, _mm_mul_ps(a, b));
#elif defined(USE_NEON)
    return vfmaq_f32(c, a, b);
#endif
}

}

#endif