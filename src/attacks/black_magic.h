#pragma once

#include "attacks/utility.h"
#include "bitboard/enum.h"

// Idea by Volker Annuss

struct MagicOffset
{
    constexpr MagicOffset(uint64_t m, size_t o) noexcept
        : magic(m)
        , offset(o)
    {
    }

    uint64_t magic;
    size_t offset;
};

struct BlackMagicBishopTraits
{
    constexpr static std::array<MagicOffset, N_SQUARES> magic_offsets = {
        MagicOffset { 0xa7020080601803d8ull, 60984 },
        MagicOffset { 0x13802040400801f1ull, 66046 },
        MagicOffset { 0x0a0080181001f60cull, 32910 },
        MagicOffset { 0x1840802004238008ull, 16369 },
        MagicOffset { 0xc03fe00100000000ull, 42115 },
        MagicOffset { 0x24c00bffff400000ull, 835 },
        MagicOffset { 0x0808101f40007f04ull, 18910 },
        MagicOffset { 0x100808201ec00080ull, 25911 },
        MagicOffset { 0xffa2feffbfefb7ffull, 63301 },
        MagicOffset { 0x083e3ee040080801ull, 16063 },
        MagicOffset { 0xc0800080181001f8ull, 17481 },
        MagicOffset { 0x0440007fe0031000ull, 59361 },
        MagicOffset { 0x2010007ffc000000ull, 18735 },
        MagicOffset { 0x1079ffe000ff8000ull, 61249 },
        MagicOffset { 0x3c0708101f400080ull, 68938 },
        MagicOffset { 0x080614080fa00040ull, 61791 },
        MagicOffset { 0x7ffe7fff817fcff9ull, 21893 },
        MagicOffset { 0x7ffebfffa01027fdull, 62068 },
        MagicOffset { 0x53018080c00f4001ull, 19829 },
        MagicOffset { 0x407e0001000ffb8aull, 26091 },
        MagicOffset { 0x201fe000fff80010ull, 15815 },
        MagicOffset { 0xffdfefffde39ffefull, 16419 },
        MagicOffset { 0xcc8808000fbf8002ull, 59777 },
        MagicOffset { 0x7ff7fbfff8203fffull, 16288 },
        MagicOffset { 0x8800013e8300c030ull, 33235 },
        MagicOffset { 0x0420009701806018ull, 15459 },
        MagicOffset { 0x7ffeff7f7f01f7fdull, 15863 },
        MagicOffset { 0x8700303010c0c006ull, 75555 },
        MagicOffset { 0xc800181810606000ull, 79445 },
        MagicOffset { 0x20002038001c8010ull, 15917 },
        MagicOffset { 0x087ff038000fc001ull, 8512 },
        MagicOffset { 0x00080c0c00083007ull, 73069 },
        MagicOffset { 0x00000080fc82c040ull, 16078 },
        MagicOffset { 0x000000407e416020ull, 19168 },
        MagicOffset { 0x00600203f8008020ull, 11056 },
        MagicOffset { 0xd003fefe04404080ull, 62544 },
        MagicOffset { 0xa00020c018003088ull, 80477 },
        MagicOffset { 0x7fbffe700bffe800ull, 75049 },
        MagicOffset { 0x107ff00fe4000f90ull, 32947 },
        MagicOffset { 0x7f8fffcff1d007f8ull, 59172 },
        MagicOffset { 0x0000004100f88080ull, 55845 },
        MagicOffset { 0x00000020807c4040ull, 61806 },
        MagicOffset { 0x00000041018700c0ull, 73601 },
        MagicOffset { 0x0010000080fc4080ull, 15546 },
        MagicOffset { 0x1000003c80180030ull, 45243 },
        MagicOffset { 0xc10000df80280050ull, 20333 },
        MagicOffset { 0xffffffbfeff80fdcull, 33402 },
        MagicOffset { 0x000000101003f812ull, 25917 },
        MagicOffset { 0x0800001f40808200ull, 32875 },
        MagicOffset { 0x084000101f3fd208ull, 4639 },
        MagicOffset { 0x080000000f808081ull, 17077 },
        MagicOffset { 0x0004000008003f80ull, 62324 },
        MagicOffset { 0x08000001001fe040ull, 18159 },
        MagicOffset { 0x72dd000040900a00ull, 61436 },
        MagicOffset { 0xfffffeffbfeff81dull, 57073 },
        MagicOffset { 0xcd8000200febf209ull, 61025 },
        MagicOffset { 0x100000101ec10082ull, 81259 },
        MagicOffset { 0x7fbaffffefe0c02full, 64083 },
        MagicOffset { 0x7f83fffffff07f7full, 56114 },
        MagicOffset { 0xfff1fffffff7ffc1ull, 57058 },
        MagicOffset { 0x0878040000ffe01full, 58912 },
        MagicOffset { 0x945e388000801012ull, 22194 },
        MagicOffset { 0x0840800080200fdaull, 70880 },
        MagicOffset { 0x100000c05f582008ull, 11140 },
    };

    constexpr static std::array<Shift, 4> directions = { Shift::NW, Shift::NE, Shift::SW, Shift::SE };
    constexpr static size_t shift = 9;
};

struct BlackMagicRookTraits
{
    constexpr static std::array<MagicOffset, N_SQUARES> magic_offsets = {
        MagicOffset { 0x80280013ff84ffffull, 10890 },
        MagicOffset { 0x5ffbfefdfef67fffull, 50579 },
        MagicOffset { 0xffeffaffeffdffffull, 62020 },
        MagicOffset { 0x003000900300008aull, 67322 },
        MagicOffset { 0x0050028010500023ull, 80251 },
        MagicOffset { 0x0020012120a00020ull, 58503 },
        MagicOffset { 0x0030006000c00030ull, 51175 },
        MagicOffset { 0x0058005806b00002ull, 83130 },
        MagicOffset { 0x7fbff7fbfbeafffcull, 50430 },
        MagicOffset { 0x0000140081050002ull, 21613 },
        MagicOffset { 0x0000180043800048ull, 72625 },
        MagicOffset { 0x7fffe800021fffb8ull, 80755 },
        MagicOffset { 0xffffcffe7fcfffafull, 69753 },
        MagicOffset { 0x00001800c0180060ull, 26973 },
        MagicOffset { 0x4f8018005fd00018ull, 84972 },
        MagicOffset { 0x0000180030620018ull, 31958 },
        MagicOffset { 0x00300018010c0003ull, 69272 },
        MagicOffset { 0x0003000c0085ffffull, 48372 },
        MagicOffset { 0xfffdfff7fbfefff7ull, 65477 },
        MagicOffset { 0x7fc1ffdffc001fffull, 43972 },
        MagicOffset { 0xfffeffdffdffdfffull, 57154 },
        MagicOffset { 0x7c108007befff81full, 53521 },
        MagicOffset { 0x20408007bfe00810ull, 30534 },
        MagicOffset { 0x0400800558604100ull, 16548 },
        MagicOffset { 0x0040200010080008ull, 46407 },
        MagicOffset { 0x0010020008040004ull, 11841 },
        MagicOffset { 0xfffdfefff7fbfff7ull, 21112 },
        MagicOffset { 0xfebf7dfff8fefff9ull, 44214 },
        MagicOffset { 0xc00000ffe001ffe0ull, 57925 },
        MagicOffset { 0x4af01f00078007c3ull, 29574 },
        MagicOffset { 0xbffbfafffb683f7full, 17309 },
        MagicOffset { 0x0807f67ffa102040ull, 40143 },
        MagicOffset { 0x200008e800300030ull, 64659 },
        MagicOffset { 0x0000008780180018ull, 70469 },
        MagicOffset { 0x0000010300180018ull, 62917 },
        MagicOffset { 0x4000008180180018ull, 60997 },
        MagicOffset { 0x008080310005fffaull, 18554 },
        MagicOffset { 0x4000188100060006ull, 14385 },
        MagicOffset { 0xffffff7fffbfbfffull, 0 },
        MagicOffset { 0x0000802000200040ull, 38091 },
        MagicOffset { 0x20000202ec002800ull, 25122 },
        MagicOffset { 0xfffff9ff7cfff3ffull, 60083 },
        MagicOffset { 0x000000404b801800ull, 72209 },
        MagicOffset { 0x2000002fe03fd000ull, 67875 },
        MagicOffset { 0xffffff6ffe7fcffdull, 56290 },
        MagicOffset { 0xbff7efffbfc00fffull, 43807 },
        MagicOffset { 0x000000100800a804ull, 73365 },
        MagicOffset { 0x6054000a58005805ull, 76398 },
        MagicOffset { 0x0829000101150028ull, 20024 },
        MagicOffset { 0x00000085008a0014ull, 9513 },
        MagicOffset { 0x8000002b00408028ull, 24324 },
        MagicOffset { 0x4000002040790028ull, 22996 },
        MagicOffset { 0x7800002010288028ull, 23213 },
        MagicOffset { 0x0000001800e08018ull, 56002 },
        MagicOffset { 0xa3a80003f3a40048ull, 22809 },
        MagicOffset { 0x2003d80000500028ull, 44545 },
        MagicOffset { 0xfffff37eefefdfbeull, 36072 },
        MagicOffset { 0x40000280090013c1ull, 4750 },
        MagicOffset { 0xbf7ffeffbffaf71full, 6014 },
        MagicOffset { 0xfffdffff777b7d6eull, 36054 },
        MagicOffset { 0x48300007e8080c02ull, 78538 },
        MagicOffset { 0xafe0000fff780402ull, 28745 },
        MagicOffset { 0xee73fffbffbb77feull, 8555 },
        MagicOffset { 0x0002000308482882ull, 1009 },
    };

    constexpr static std::array<Shift, 4> directions = { Shift::N, Shift::S, Shift::W, Shift::E };
    constexpr static size_t shift = 12;
};

struct BlackMagicTraits
{
    using BishopTraits = BlackMagicBishopTraits;
    using RookTraits = BlackMagicRookTraits;

    constexpr static size_t table_size = 87988;
};

template <typename Traits>
struct BlackMagicStrategy
{
    struct Metadata
    {
        size_t index;
        uint64_t notmask;
        uint64_t magic;
    };

    std::array<uint64_t, Traits::table_size> attacks = {};
    std::array<Metadata, N_SQUARES> rook_metadata = {};
    std::array<Metadata, N_SQUARES> bishop_metadata = {};

    BlackMagicStrategy()
    {
        init_slider<typename Traits::RookTraits>(rook_metadata);
        init_slider<typename Traits::BishopTraits>(bishop_metadata);
    }

    template <typename PieceTraits>
    void init_slider(std::array<Metadata, N_SQUARES>& metadata)
    {
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            metadata[sq].index = PieceTraits::magic_offsets[sq].offset;
            metadata[sq].magic = PieceTraits::magic_offsets[sq].magic;

            // Construct the mask
            uint64_t edges = ((RankBB[RANK_1] | RankBB[RANK_8]) & ~RankBB[enum_to<Rank>(sq)])
                | ((FileBB[FILE_A] | FileBB[FILE_H]) & ~FileBB[enum_to<File>(sq)]);
            const auto mask = make_slider_attacks_bb<PieceTraits::directions>(sq, 0) & ~edges;
            metadata[sq].notmask = ~mask;

            uint64_t occupied = 0;
            do
            {
                attack_mask<PieceTraits::shift>(sq, occupied, metadata)
                    = make_slider_attacks_bb<PieceTraits::directions>(sq, occupied);
                occupied = (occupied - mask) & mask; // Carry rippler
            } while (occupied);
        }
    }

    template <size_t shift>
    constexpr uint64_t& attack_mask(Square sq, uint64_t occupied, const std::array<Metadata, N_SQUARES>& metadata) noexcept
    {
        const size_t offset = ((occupied | metadata[sq].notmask) * metadata[sq].magic) >> (64 - shift);
        return attacks[metadata[sq].index + offset];
    }

    template <size_t shift>
    constexpr uint64_t attack_mask(Square sq, uint64_t occupied, const std::array<Metadata, N_SQUARES>& metadata) const noexcept
    {
        return const_cast<std::remove_const_t<std::remove_pointer_t<decltype(this)>>*>(this)
            ->template attack_mask<shift>(sq, occupied, metadata);
    }
};

// To keep a unified magic interface, we need to pretend that the black magic tables aren't shared between rook/bishop.
// We use a function static variable to create it once on demand, then wrap it in accessors

template <typename Traits>
const BlackMagicStrategy<Traits>& get_shared_black_magic_strategy()
{
    static BlackMagicStrategy<Traits> strategy;
    return strategy;
}

template <typename Traits>
struct BlackMagicStrategyRookAccessor
{
    const BlackMagicStrategy<Traits>& strategy = get_shared_black_magic_strategy<Traits>();

    constexpr uint64_t attack_mask(Square sq, uint64_t occupied) const noexcept
    {
        return strategy.template attack_mask<Traits::RookTraits::shift>(sq, occupied, strategy.rook_metadata);
    }
};

template <typename Traits>
struct BlackMagicStrategyBishopAccessor
{
    const BlackMagicStrategy<Traits>& strategy = get_shared_black_magic_strategy<Traits>();

    constexpr uint64_t attack_mask(Square sq, uint64_t occupied) const noexcept
    {
        return strategy.template attack_mask<Traits::BishopTraits::shift>(sq, occupied, strategy.bishop_metadata);
    }
};

// 690 KB combined
using BishopBlackMagicStrategy = BlackMagicStrategyBishopAccessor<BlackMagicTraits>;
using RookBlackMagicStrategy = BlackMagicStrategyRookAccessor<BlackMagicTraits>;