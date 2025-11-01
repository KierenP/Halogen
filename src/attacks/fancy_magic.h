#pragma once

#include "attacks/utility.h"

struct FancyMagicBishopTraits
{
    constexpr static std::array<uint64_t, N_SQUARES> magics = {
        // clang-format off
        0xFFEDF9FD7CFCFFFFULL, 0xFC0962854A77F576ULL, 0x5822022042000000ULL, 0x2CA804A100200020ULL,
        0x0204042200000900ULL, 0x2002121024000002ULL, 0xFC0A66C64A7EF576ULL, 0x7FFDFDFCBD79FFFFULL,
        0xFC0846A64A34FFF6ULL, 0xFC087A874A3CF7F6ULL, 0x1001080204002100ULL, 0x1810080489021800ULL,
        0x0062040420010A00ULL, 0x5028043004300020ULL, 0xFC0864AE59B4FF76ULL, 0x3C0860AF4B35FF76ULL,
        0x73C01AF56CF4CFFBULL, 0x41A01CFAD64AAFFCULL, 0x040C0422080A0598ULL, 0x4228020082004050ULL,
        0x0200800400E00100ULL, 0x020B001230021040ULL, 0x7C0C028F5B34FF76ULL, 0xFC0A028E5AB4DF76ULL,
        0x0020208050A42180ULL, 0x001004804B280200ULL, 0x2048020024040010ULL, 0x0102C04004010200ULL,
        0x020408204C002010ULL, 0x02411100020080C1ULL, 0x102A008084042100ULL, 0x0941030000A09846ULL,
        0x0244100800400200ULL, 0x4000901010080696ULL, 0x0000280404180020ULL, 0x0800042008240100ULL,
        0x0220008400088020ULL, 0x04020182000904C9ULL, 0x0023010400020600ULL, 0x0041040020110302ULL,
        0xDCEFD9B54BFCC09FULL, 0xF95FFA765AFD602BULL, 0x1401210240484800ULL, 0x0022244208010080ULL,
        0x1105040104000210ULL, 0x2040088800C40081ULL, 0x43FF9A5CF4CA0C01ULL, 0x4BFFCD8E7C587601ULL,
        0xFC0FF2865334F576ULL, 0xFC0BF6CE5924F576ULL, 0x80000B0401040402ULL, 0x0020004821880A00ULL,
        0x8200002022440100ULL, 0x0009431801010068ULL, 0xC3FFB7DC36CA8C89ULL, 0xC3FF8A54F4CA2C89ULL,
        0xFFFFFCFCFD79EDFFULL, 0xFC0863FCCB147576ULL, 0x040C000022013020ULL, 0x2000104000420600ULL,
        0x0400000260142410ULL, 0x0800633408100500ULL, 0xFC087E8E4BB2F736ULL, 0x43FF9E4EF4CA2C89ULL,
        // clang-format on
    };

    constexpr static size_t table_size = 0x1480;
    constexpr static std::array<Shift, 4> directions = { Shift::NW, Shift::NE, Shift::SW, Shift::SE };
};

struct FancyMagicRookTraits
{
    constexpr static std::array<uint64_t, N_SQUARES> magics = {
        // clang-format off
        0xA180022080400230ULL, 0x0040100040022000ULL, 0x0080088020001002ULL, 0x0080080280841000ULL,
        0x4200042010460008ULL, 0x04800A0003040080ULL, 0x0400110082041008ULL, 0x008000A041000880ULL,
        0x10138001A080C010ULL, 0x0000804008200480ULL, 0x00010011012000C0ULL, 0x0022004128102200ULL,
        0x000200081201200CULL, 0x202A001048460004ULL, 0x0081000100420004ULL, 0x4000800380004500ULL,
        0x0000208002904001ULL, 0x0090004040026008ULL, 0x0208808010002001ULL, 0x2002020020704940ULL,
        0x8048010008110005ULL, 0x6820808004002200ULL, 0x0A80040008023011ULL, 0x00B1460000811044ULL,
        0x4204400080008EA0ULL, 0xB002400180200184ULL, 0x2020200080100380ULL, 0x0010080080100080ULL,
        0x2204080080800400ULL, 0x0000A40080360080ULL, 0x02040604002810B1ULL, 0x008C218600004104ULL,
        0x8180004000402000ULL, 0x488C402000401001ULL, 0x4018A00080801004ULL, 0x1230002105001008ULL,
        0x8904800800800400ULL, 0x0042000C42003810ULL, 0x008408110400B012ULL, 0x0018086182000401ULL,
        0x2240088020C28000ULL, 0x001001201040C004ULL, 0x0A02008010420020ULL, 0x0010003009010060ULL,
        0x0004008008008014ULL, 0x0080020004008080ULL, 0x0282020001008080ULL, 0x50000181204A0004ULL,
        0x48FFFE99FECFAA00ULL, 0x48FFFE99FECFAA00ULL, 0x497FFFADFF9C2E00ULL, 0x613FFFDDFFCE9200ULL,
        0xFFFFFFE9FFE7CE00ULL, 0xFFFFFFF5FFF3E600ULL, 0x0010301802830400ULL, 0x510FFFF5F63C96A0ULL,
        0xEBFFFFB9FF9FC526ULL, 0x61FFFEDDFEEDAEAEULL, 0x53BFFFEDFFDEB1A2ULL, 0x127FFFB9FFDFB5F6ULL,
        0x411FFFDDFFDBF4D6ULL, 0x0801000804000603ULL, 0x0003FFEF27EEBE74ULL, 0x7645FFFECBFEA79EULL,
        // clang-format on
    };

    constexpr static size_t table_size = 0x19000;
    constexpr static std::array<Shift, 4> directions = { Shift::N, Shift::S, Shift::W, Shift::E };
};

template <typename Traits>
struct FancyMagicStrategy
{
    struct Metadata
    {
        size_t index;
        uint64_t mask;
        int shift;
    };

    std::array<uint64_t, Traits::table_size> attacks = {};
    std::array<Metadata, N_SQUARES> metadata = {};

    FancyMagicStrategy()
    {
        size_t attack_index = 0;
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            metadata[sq].index = attack_index;

            // Construct the mask
            uint64_t edges = ((RankBB[RANK_1] | RankBB[RANK_8]) & ~RankBB[enum_to<Rank>(sq)])
                | ((FileBB[FILE_A] | FileBB[FILE_H]) & ~FileBB[enum_to<File>(sq)]);

            metadata[sq].mask = make_slider_attacks_bb<Traits::directions>(sq, 0) & ~edges;
            metadata[sq].shift = 64 - std::popcount(metadata[sq].mask);

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
        const size_t offset = ((occupied & metadata[sq].mask) * Traits::magics[sq]) >> metadata[sq].shift;
        return attacks[metadata[sq].index + offset];
    }

    constexpr uint64_t attack_mask(Square sq, uint64_t occupied) const
    {
        return const_cast<std::remove_const_t<std::remove_pointer_t<decltype(this)>>*>(this)->attack_mask(sq, occupied);
    }
};