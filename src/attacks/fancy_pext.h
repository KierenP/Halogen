#pragma once

#include "attacks/utility.h"
#include <immintrin.h>

struct FancyPEXTRookTraits
{
    constexpr static size_t table_size = 0x19000;
    constexpr static std::array<Shift, 4> directions = { Shift::N, Shift::S, Shift::W, Shift::E };
};

struct FancyPEXTBishopTraits
{
    constexpr static size_t table_size = 0x1480;
    constexpr static std::array<Shift, 4> directions = { Shift::NW, Shift::NE, Shift::SW, Shift::SE };
};

template <typename Traits>
struct FancyPEXTStrategy
{
    struct Metadata
    {
        size_t index;
        uint64_t mask;
    };

    std::array<uint64_t, Traits::table_size> attacks = {};
    std::array<Metadata, N_SQUARES> metadata = {};

    FancyPEXTStrategy()
    {
        size_t attack_index = 0;
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            metadata[sq].index = attack_index;

            // Construct the mask
            uint64_t edges = ((RankBB[RANK_1] | RankBB[RANK_8]) & ~RankBB[enum_to<Rank>(sq)])
                | ((FileBB[FILE_A] | FileBB[FILE_H]) & ~FileBB[enum_to<File>(sq)]);

            metadata[sq].mask = make_slider_attacks_bb<Traits::directions>(sq, 0) & ~edges;

            uint64_t occupied = 0;
            do
            {
                attack_mask(sq, occupied) = make_slider_attacks_bb<Traits::directions>(sq, occupied);
                occupied = (occupied - metadata[sq].mask) & metadata[sq].mask; // Carry rippler
                attack_index++;
            } while (occupied);
        }
    }

    constexpr uint64_t& attack_mask(Square sq, uint64_t occupied)
    {
        const size_t offset = _pext_u64(occupied, metadata[sq].mask);
        return attacks[metadata[sq].index + offset];
    }

    constexpr uint64_t attack_mask(Square sq, uint64_t occupied) const
    {
        return const_cast<std::remove_const_t<std::remove_pointer_t<decltype(this)>>*>(this)->attack_mask(sq, occupied);
    }
};

// 42KB
using BishopFancyPEXTStrategy = FancyPEXTStrategy<FancyPEXTBishopTraits>;

// 801KB
using RookFancyPEXTStrategy = FancyPEXTStrategy<FancyPEXTRookTraits>;
