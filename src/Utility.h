#pragma once

#include "BitBoardDefine.h"
#include "Bitboard.h"

template <typename Fn>
void extract_bits(BB bb, Fn&& fn)
{
    while (bb != BB::none)
    {
        fn(LSBpop(bb));
    }
}

template <typename Fn>
[[nodiscard]] bool any_of(BB bb, Fn&& fn)
{
    while (bb != BB::none)
    {
        if (fn(LSBpop(bb)))
        {
            return true;
        }
    }

    return false;
}