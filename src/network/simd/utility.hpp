#pragma once

#include "network/arch.hpp"
#include "simd/intrinsics.hpp"
#include "simd/utility.hpp"

#if defined(USE_SSE4)
#include <immintrin.h>
#endif

#if defined(USE_NEON)
#include <arm_neon.h>
#endif

#include <cstdint>

#include "bitboard/define.h"

namespace NN::SIMD
{

#if defined(SIMD_ENABLED)
// Make the generic SIMD layer visible as NN::SIMD, so NNUE code can use unqualified or SIMD:: qualified names.
using namespace ::SIMD;
#endif

// For the expensive FT -> L1 matmul, we want to do a sparse multiplication. The naive approach would be to construct a
// vector of non zero FT activations, and then multiply by the corresponding L1 weights. Because we want to accumulate
// u8*i8 into i32 using two madd instructions, it makes it much easier if we work in groups of 4 FT activations at a
// time (called a FT nibble). Hence, we check blocks of 4 adjacent FT activations in a stride and omit any blocks of 4
// zeros.
//
// In order to efficiently encode the sparse FT nibbles, we process them in blocks of 8 at a time (8 * 4 = 32). Using a
// movemask instruction along with a greater than, we can extract one byte at a time to represent the sparsity of 8 FT
// nibbles. We precompute a 256 entry lookup table to map to the corresponding mask. For example, if we the mask is
// 0b10001001, then we would extract indicies = { 0, 3, 7 }. We store each index in a u16, so that 8 of them occupy
// a _m128i register. This allows us to very quickly apply a offset using a add_epi16 to efficiently map the indicies to
// the correct FT nibbles.

struct SparseAffineEntry
{
    alignas(16) std::array<int16_t, 8> indicies;
    size_t count;
};

struct SparseAffineTable
{
    SparseAffineTable()
    {
        for (uint64_t i = 0; i < 256; i++)
        {
            uint64_t bits = i;
            size_t idx = 0;
            while (bits)
            {
                entry[i].indicies[idx++] = lsbpop(bits);
            }
            entry[i].count = idx;
        }
    }

    std::array<SparseAffineEntry, 256> entry;
};

const SparseAffineTable sparse_affine_table;

#if defined(USE_AVX512_VNNI)
inline void deposit_nonzero_4xu8_block_indices_x2(vecu8 a, vecu8 b, veci16& offset,
    std::array<int16_t, FT_SIZE / 4>& sparse_block_indices, size_t& sparse_block_indices_size)
{
    alignas(64) static constexpr std::array<int16_t, 32> index_table = []
    {
        std::array<int16_t, 32> cache {};
        for (int16_t i = 0; i < 32; ++i)
        {
            cache[i] = i;
        }
        return cache;
    }();

    auto mask1 = SIMD::cmpgt_i32_mask(a);
    auto mask2 = SIMD::cmpgt_i32_mask(b);
    uint32_t mask = mask1 | (mask2 << 16);
    auto indicies = SIMD::load(index_table.data());
    indicies = SIMD::add_i16(indicies, offset);
    assert(sparse_block_indices_size + std::popcount(mask) <= sparse_block_indices.size());
    _mm512_mask_compressstoreu_epi16(&sparse_block_indices[sparse_block_indices_size], mask, indicies);
    sparse_block_indices_size += std::popcount(mask);
    offset = SIMD::add_i16(offset, SIMD::set_i16(32));
}
inline void deposit_nonzero_4xu8_block_indices(
    vecu8 a, veci16& offset, std::array<int16_t, FT_SIZE / 4>& sparse_block_indices, size_t& sparse_block_indices_size)
{
    alignas(64) static constexpr std::array<int16_t, 32> index_table = []
    {
        std::array<int16_t, 32> cache {};
        for (int16_t i = 0; i < 16; ++i)
        {
            cache[i] = i;
        }
        return cache;
    }();

    auto mask = SIMD::cmpgt_i32_mask(a);
    auto indicies = SIMD::load(index_table.data());
    indicies = SIMD::add_i16(indicies, offset);
    assert(sparse_block_indices_size + std::popcount(mask) <= sparse_block_indices.size());
    _mm512_mask_compressstoreu_epi16(&sparse_block_indices[sparse_block_indices_size], mask, indicies);
    sparse_block_indices_size += std::popcount(mask);
    offset = SIMD::add_i16(offset, SIMD::set_i16(16));
}
#elif defined(USE_NEON)
inline void deposit_nonzero_4xu8_block_indices_x2(vecu8 a, vecu8 b, veci16& offset,
    std::array<int16_t, FT_SIZE / 4>& sparse_block_indices, size_t& sparse_block_indices_size)
{
    auto mask1 = SIMD::cmpgt_i32_mask(a);
    auto mask2 = SIMD::cmpgt_i32_mask(b);
    uint8_t mask = mask1 | (mask2 << 4);
    const auto& entry = sparse_affine_table.entry[mask];
    auto indicies = vld1q_s16(entry.indicies.data());
    indicies = SIMD::add_i16(indicies, offset);
    assert(sparse_block_indices_size + entry.count <= sparse_block_indices.size());
    vst1q_s16(&sparse_block_indices[sparse_block_indices_size], indicies);
    sparse_block_indices_size += entry.count;
    offset = SIMD::add_i16(offset, SIMD::set_i16(8));
}
inline void deposit_nonzero_4xu8_block_indices(
    vecu8 a, veci16& offset, std::array<int16_t, FT_SIZE / 4>& sparse_block_indices, size_t& sparse_block_indices_size)
{
    auto mask = SIMD::cmpgt_i32_mask(a);
    const auto& entry = sparse_affine_table.entry[mask];
    auto indicies = vld1q_s16(entry.indicies.data());
    indicies = SIMD::add_i16(indicies, offset);
    assert(sparse_block_indices_size + entry.count <= sparse_block_indices.size());
    vst1q_s16(&sparse_block_indices[sparse_block_indices_size], indicies);
    sparse_block_indices_size += entry.count;
    offset = SIMD::add_i16(offset, SIMD::set_i16(4));
}
#elif defined(SIMD_ENABLED)
inline void deposit_nonzero_4xu8_block_indices_x2(vecu8 a, vecu8 b, __m128i& offset,
    std::array<int16_t, FT_SIZE / 4>& sparse_block_indices, size_t& sparse_block_indices_size)
{
    auto mask1 = SIMD::cmpgt_i32_mask(a);
    auto mask2 = SIMD::cmpgt_i32_mask(b);
#if defined(USE_AVX512)
    uint32_t mask = mask1 | (mask2 << 16);
#elif defined(USE_AVX2)
    uint16_t mask = mask1 | (mask2 << 8);
#else
    uint8_t mask = mask1 | (mask2 << 4);
#endif
    // 1 byte for SSE, 2 for AVX2, 4 for AVX512
    for (size_t j = 0; j < sizeof(mask); j++)
    {
        uint8_t bytemask = (mask >> (8 * j)) & 0xFF;
        const auto& entry = sparse_affine_table.entry[bytemask];
        auto indicies = _mm_load_si128(reinterpret_cast<const __m128i*>(entry.indicies.data()));
        indicies = _mm_add_epi16(indicies, offset);
        assert(sparse_block_indices_size + entry.count <= sparse_block_indices.size());
        auto* sparse_block_indices_end = reinterpret_cast<__m128i*>(&sparse_block_indices[sparse_block_indices_size]);
        _mm_storeu_si128(sparse_block_indices_end, indicies);
        sparse_block_indices_size += entry.count;
        offset = _mm_add_epi16(offset, _mm_set1_epi16(8));
    }
}
inline void deposit_nonzero_4xu8_block_indices(
    vecu8 a, __m128i& offset, std::array<int16_t, FT_SIZE / 4>& sparse_block_indices, size_t& sparse_block_indices_size)
{
    auto mask = SIMD::cmpgt_i32_mask(a);
#if defined(USE_AVX512)
    uint16_t umask = mask;
#elif defined(USE_AVX2)
    uint8_t umask = mask;
#else
    uint8_t umask = mask;
#endif
    // 1 byte for SSE, 1 for AVX2, 2 for AVX512
    for (size_t j = 0; j < sizeof(umask); j++)
    {
        uint8_t bytemask = (umask >> (8 * j)) & 0xFF;
        const auto& entry = sparse_affine_table.entry[bytemask];
        auto indicies = _mm_load_si128(reinterpret_cast<const __m128i*>(entry.indicies.data()));
        indicies = _mm_add_epi16(indicies, offset);
        assert(sparse_block_indices_size + entry.count <= sparse_block_indices.size());
        auto* sparse_block_indices_end = reinterpret_cast<__m128i*>(&sparse_block_indices[sparse_block_indices_size]);
        _mm_storeu_si128(sparse_block_indices_end, indicies);
        sparse_block_indices_size += entry.count;
#if defined(USE_AVX512) || defined(USE_AVX2)
        offset = _mm_add_epi16(offset, _mm_set1_epi16(8));
#else
        offset = _mm_add_epi16(offset, _mm_set1_epi16(4));
#endif
    }
}
#endif
}
