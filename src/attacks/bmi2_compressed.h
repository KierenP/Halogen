#pragma once

#include "attacks/utility.h"

struct BMI2CompressedRookTraits
{
    constexpr static size_t table_size = 0x19000;
    constexpr static std::array<Shift, 4> directions = { Shift::N, Shift::S, Shift::W, Shift::E };
};

struct BMI2CompressedBishopTraits
{
    constexpr static size_t table_size = 0x1480;
    constexpr static std::array<Shift, 4> directions = { Shift::NW, Shift::NE, Shift::SW, Shift::SE };
};

// Idea by Zach Wegner, we can use pdep to store compressed attack bitboards which saves 75% space
template <typename Traits>
struct BMI2CompressedStrategy
{
    struct Metadata
    {
        size_t index;
        uint64_t src_mask;
        uint64_t dst_mask;
    };

    std::array<uint16_t, Traits::table_size> attacks = {};
    std::array<Metadata, N_SQUARES> metadata = {};

    BMI2CompressedStrategy()
    {
        size_t attack_index = 0;
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            metadata[sq].index = attack_index;

            // Construct the mask
            uint64_t edges = ((RankBB[RANK_1] | RankBB[RANK_8]) & ~RankBB[enum_to<Rank>(sq)])
                | ((FileBB[FILE_A] | FileBB[FILE_H]) & ~FileBB[enum_to<File>(sq)]);

            metadata[sq].dst_mask = make_slider_attacks_bb<Traits::directions>(sq, 0);
            metadata[sq].src_mask = metadata[sq].dst_mask & ~edges;

            uint64_t occupied = 0;
            do
            {
                auto attack_mask = make_slider_attacks_bb<Traits::directions>(sq, occupied);
                compressed_mask(sq, occupied) = _pext_u64(static_cast<uint64_t>(attack_mask), metadata[sq].dst_mask);
                occupied = (occupied - metadata[sq].src_mask) & metadata[sq].src_mask; // Carry rippler
                attack_index++;
            } while (occupied);
        }
    }

    constexpr uint64_t attack_mask(Square sq, uint64_t occupied) const
    {
        const size_t offset = _pext_u64(occupied, metadata[sq].src_mask);
        const auto& compressed_attacks = attacks[metadata[sq].index + offset];
        return _pdep_u64(static_cast<uint64_t>(compressed_attacks), metadata[sq].dst_mask);
    }

    constexpr uint16_t& compressed_mask(Square sq, uint64_t occupied)
    {
        const size_t offset = _pext_u64(occupied, metadata[sq].src_mask);
        return attacks[metadata[sq].index + offset];
    }
};