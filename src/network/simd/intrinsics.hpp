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
    if constexpr (std::is_same_v<T, int8_t>)
    {
#if defined(USE_AVX512)
        return _mm512_load_si512(ptr);
#elif defined(USE_AVX2)
        return _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
#elif defined(USE_SSE4)
        return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
#elif defined(USE_NEON)
        return vld1q_s8(ptr);
#endif
    }
    else if constexpr (std::is_same_v<T, uint8_t>)
    {
#if defined(USE_AVX512)
        return _mm512_load_si512(ptr);
#elif defined(USE_AVX2)
        return _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
#elif defined(USE_SSE4)
        return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
#elif defined(USE_NEON)
        return vld1q_u8(ptr);
#endif
    }
    else if constexpr (std::is_same_v<T, int16_t>)
    {
#if defined(USE_AVX512)
        return _mm512_load_si512(ptr);
#elif defined(USE_AVX2)
        return _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
#elif defined(USE_SSE4)
        return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
#elif defined(USE_NEON)
        return vld1q_s16(ptr);
#endif
    }
    else if constexpr (std::is_same_v<T, int32_t>)
    {
#if defined(USE_AVX512)
        return _mm512_load_si512(ptr);
#elif defined(USE_AVX2)
        return _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
#elif defined(USE_SSE4)
        return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
#elif defined(USE_NEON)
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
    if constexpr (std::is_same_v<T, int8_t>)
    {
#if defined(USE_AVX512)
        _mm512_store_si512(ptr, v);
#elif defined(USE_AVX2)
        _mm256_store_si256(reinterpret_cast<__m256i*>(ptr), v);
#elif defined(USE_SSE4)
        _mm_store_si128(reinterpret_cast<__m128i*>(ptr), v);
#elif defined(USE_NEON)
        vst1q_s8(ptr, v);
#endif
    }
    else if constexpr (std::is_same_v<T, uint8_t>)
    {
#if defined(USE_AVX512)
        _mm512_store_si512(ptr, v);
#elif defined(USE_AVX2)
        _mm256_store_si256(reinterpret_cast<__m256i*>(ptr), v);
#elif defined(USE_SSE4)
        _mm_store_si128(reinterpret_cast<__m128i*>(ptr), v);
#elif defined(USE_NEON)
        vst1q_u8(ptr, v);
#endif
    }
    else if constexpr (std::is_same_v<T, int16_t>)
    {
#if defined(USE_AVX512)
        _mm512_store_si512(ptr, v);
#elif defined(USE_AVX2)
        _mm256_store_si256(reinterpret_cast<__m256i*>(ptr), v);
#elif defined(USE_SSE4)
        _mm_store_si128(reinterpret_cast<__m128i*>(ptr), v);
#elif defined(USE_NEON)
        vst1q_s16(ptr, v);
#endif
    }
    else if constexpr (std::is_same_v<T, int32_t>)
    {
#if defined(USE_AVX512)
        _mm512_store_si512(ptr, v);
#elif defined(USE_AVX2)
        _mm256_store_si256(reinterpret_cast<__m256i*>(ptr), v);
#elif defined(USE_SSE4)
        _mm_store_si128(reinterpret_cast<__m128i*>(ptr), v);
#elif defined(USE_NEON)
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
#if defined(USE_AVX512)
    if constexpr (std::is_same_v<T, __m512i> || std::is_same_v<T, veci16> || std::is_same_v<T, veci32>
        || std::is_same_v<T, veci8> || std::is_same_v<T, vecu8>)
        return _mm512_setzero_si512();
    else if constexpr (std::is_same_v<T, __m512> || std::is_same_v<T, vecf32>)
        return _mm512_setzero_ps();
#elif defined(USE_AVX2)
    if constexpr (std::is_same_v<T, __m256i> || std::is_same_v<T, veci16> || std::is_same_v<T, veci32>
        || std::is_same_v<T, veci8> || std::is_same_v<T, vecu8>)
        return _mm256_setzero_si256();
    else if constexpr (std::is_same_v<T, __m256> || std::is_same_v<T, vecf32>)
        return _mm256_setzero_ps();
#elif defined(USE_SSE4)
    if constexpr (std::is_same_v<T, __m128i> || std::is_same_v<T, veci16> || std::is_same_v<T, veci32>
        || std::is_same_v<T, veci8> || std::is_same_v<T, vecu8>)
        return _mm_setzero_si128();
    else if constexpr (std::is_same_v<T, __m128> || std::is_same_v<T, vecf32>)
        return _mm_setzero_ps();
#elif defined(USE_NEON)
    if constexpr (std::is_same_v<T, int16x8_t> || std::is_same_v<T, veci16>)
        return vdupq_n_s16(0);
    else if constexpr (std::is_same_v<T, int32x4_t> || std::is_same_v<T, veci32>)
        return vdupq_n_s32(0);
    else if constexpr (std::is_same_v<T, int8x16_t> || std::is_same_v<T, veci8>)
        return vdupq_n_s8(0);
    else if constexpr (std::is_same_v<T, uint8x16_t> || std::is_same_v<T, vecu8>)
        return vdupq_n_u8(0);
    else if constexpr (std::is_same_v<T, float32x4_t> || std::is_same_v<T, vecf32>)
        return vdupq_n_f32(0.0f);
#endif
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
inline veci16 slli_i16(const veci16& a)
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
    // TODO: NEON doesn't have a direct equivalent, is there a better way?
    int32x4_t lo_low = vmull_s16(vget_low_s16(a), vget_low_s16(b));
    int32x4_t lo_high = vmull_high_s16(a, b);
    int16x4_t res_low = vshrn_n_s32(lo_low, 16);
    int16x4_t res_high = vshrn_n_s32(lo_high, 16);
    return vcombine_s16(res_low, res_high);
#endif
}

inline vecu8 packus_i16(const veci16& a, const veci16& b)
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

inline uint16_t cmpgt_i32_mask(const veci32& a)
{
#if defined(USE_AVX512)
    return _mm512_cmpgt_epi32_mask(a, _mm512_setzero_si512());
#elif defined(USE_AVX2)
    return _mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(a, _mm256_setzero_si256())));
#elif defined(USE_SSE4)
    return _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(a, _mm_setzero_si128())));
#elif defined(USE_NEON)
    // TODO: NEON doesn't have a direct equivalent, is there a better way?
    uint32x4_t cmp = vcgtq_s32(a, vdupq_n_s32(0));
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
    // TODO: NEON doesn't have a direct equivalent, is there a better way?

    // Emulate maddubs: multiply u8*i8 pairs, then horizontal add pairs to i16
    // Then emulate madd: horizontal add pairs of i16 to i32
    // Net result: groups of 4 u8*i8 products summed into each i32

    // Split into 8-byte halves
    uint8x8_t a_low = vget_low_u8(a);
    uint8x8_t a_high = vget_high_u8(a);
    int8x8_t b_low = vget_low_s8(b);
    int8x8_t b_high = vget_high_s8(b);

    // Widen to 16-bit and multiply
    int16x8_t mul_low = vmulq_s16(vreinterpretq_s16_u16(vmovl_u8(a_low)), vmovl_s8(b_low));
    int16x8_t mul_high = vmulq_s16(vreinterpretq_s16_u16(vmovl_u8(a_high)), vmovl_s8(b_high));

    // Now we have:
    // mul_low:  [a0*b0, a1*b1, a2*b2, a3*b3, a4*b4, a5*b5, a6*b6, a7*b7]
    // mul_high: [a8*b8, a9*b9, a10*b10, a11*b11, a12*b12, a13*b13, a14*b14, a15*b15]

    // Pairwise add within each vector: 8xi16 -> 4xi32
    int32x4_t acc_low = vpaddlq_s16(mul_low); // [a0*b0+a1*b1, a2*b2+a3*b3, a4*b4+a5*b5, a6*b6+a7*b7]
    int32x4_t acc_high = vpaddlq_s16(mul_high); // [a8*b8+a9*b9, a10*b10+a11*b11, a12*b12+a13*b13, a14*b14+a15*b15]

    // Pairwise add again to get groups of 4
    int32x2_t sum_low = vpadd_s32(vget_low_s32(acc_low), vget_high_s32(acc_low));
    int32x2_t sum_high = vpadd_s32(vget_low_s32(acc_high), vget_high_s32(acc_high));

    // Combine into final result: 4 int32 values
    int32x4_t result = vcombine_s32(sum_low, sum_high);

    return vaddq_s32(source, result);
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