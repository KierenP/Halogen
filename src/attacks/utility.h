#pragma once

#include "bitboard/define.h"

template <Shift direction>
constexpr uint64_t make_ray_attack_bb(Square sq, uint64_t occupied) noexcept
{
    uint64_t attacks = 0;
    uint64_t bb = SquareBB[sq];

    while (true)
    {
        bb = shift_bb<direction>(bb);
        if (bb == EMPTY)
        {
            break;
        }
        attacks |= bb;
        if ((occupied & bb) != 0)
        {
            break;
        }
    }

    return attacks;
}

template <std::array<Shift, 4> directions>
constexpr uint64_t make_slider_attacks_bb(Square sq, uint64_t occupied) noexcept
{
    return (make_ray_attack_bb<directions[0]>(sq, occupied) | make_ray_attack_bb<directions[1]>(sq, occupied)
        | make_ray_attack_bb<directions[2]>(sq, occupied) | make_ray_attack_bb<directions[3]>(sq, occupied));
}