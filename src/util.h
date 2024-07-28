#pragma once

#include "BitBoardDefine.h"

template <typename T>
class Sided
{
public:
    constexpr const T& operator[](Players p) const
    {
        return data[p];
    }

    constexpr T& operator[](Players p)
    {
        return data[p];
    }

private:
    std::array<T, N_PLAYERS> data = {};
};
