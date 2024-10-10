#pragma once

#include "Define.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>

class BB
{
public:
    constexpr BB() = default;

    explicit constexpr BB(uint64_t mask)
        : mask_(mask)
    {
    }

    explicit constexpr operator uint64_t() const
    {
        return mask_;
    }

    constexpr BB operator|(BB rhs) const
    {
        return BB(mask_ | rhs.mask_);
    }

    constexpr BB operator^(BB rhs) const
    {
        return BB(mask_ ^ rhs.mask_);
    }

    constexpr BB operator&(BB rhs) const
    {
        return BB(mask_ & rhs.mask_);
    }

    constexpr BB operator~() const
    {
        return BB(~mask_);
    }

    constexpr BB& operator|=(BB rhs)
    {
        mask_ |= rhs.mask_;
        return *this;
    }

    constexpr BB& operator^=(BB rhs)
    {
        mask_ ^= rhs.mask_;
        return *this;
    }

    constexpr BB& operator&=(BB rhs)
    {
        mask_ &= rhs.mask_;
        return *this;
    }

    constexpr BB operator<<(size_t shift) const
    {
        return BB(mask_ << shift);
    }

    constexpr BB operator>>(size_t shift) const
    {
        return BB(mask_ >> shift);
    }

    constexpr BB& operator<<=(size_t shift)
    {
        mask_ <<= shift;
        return *this;
    }

    constexpr BB& operator>>=(size_t shift)
    {
        mask_ >>= shift;
        return *this;
    }

    constexpr bool operator==(const BB& rhs) const = default;

    const static BB none;
    const static BB all;

    friend std::ostream& operator<<(std::ostream& os, BB b)
    {
        std::ios::fmtflags f(std::cout.flags());
        return os << std::hex << "0x" << b.mask_;
        std::cout.flags(f);
    }

    constexpr bool contains(Square sq) const
    {
        return mask_ & (1ULL << sq);
    }

private:
    uint64_t mask_;
};

inline constexpr BB BB::none(0);
inline constexpr BB BB::all(0xFFFFFFFFFFFFFFFF);
