#pragma once
#include <cmath>
#include <cstdint>
#include <numeric>
#include <ostream>

// Helper to check if a number is a power of two
constexpr bool is_power_of_two(int64_t n)
{
    return n > 0 && (n & (n - 1)) == 0;
}

template <int64_t Precision>
class Fraction
{
    static_assert(Precision > 0, "Precision must be positive");
    static_assert(is_power_of_two(Precision), "Precision must be a power of two");
    int64_t value_; // stores the value as an integer multiple of 1/Precision

public:
    // Constructors
    constexpr Fraction()
        : value_(0)
    {
    }

    constexpr Fraction(int v)
        : value_(static_cast<int64_t>(v) * Precision)
    {
    }

    [[nodiscard]] constexpr int to_int() const
    {
        return value_ / Precision;
    }

    // Get raw value
    [[nodiscard]] constexpr int64_t raw() const
    {
        return value_;
    }

    // Get precision
    [[nodiscard]] static constexpr int64_t precision()
    {
        return Precision;
    }

    // Arithmetic operators with same precision (no loss possible)
    constexpr Fraction operator+(const Fraction& rhs) const
    {
        return Fraction::from_raw(value_ + rhs.value_);
    }
    constexpr Fraction operator-(const Fraction& rhs) const
    {
        return Fraction::from_raw(value_ - rhs.value_);
    }

    // Arithmetic operators with int
    constexpr Fraction operator+(int rhs) const
    {
        return Fraction::from_raw(value_ + static_cast<int64_t>(rhs) * Precision);
    }
    constexpr Fraction operator-(int rhs) const
    {
        return Fraction::from_raw(value_ - static_cast<int64_t>(rhs) * Precision);
    }
    constexpr Fraction operator*(int rhs) const
    {
        return Fraction::from_raw(value_ * rhs);
    }

    // Compound assignment operators
    constexpr Fraction& operator+=(const Fraction& rhs)
    {
        value_ += rhs.value_;
        return *this;
    }
    constexpr Fraction& operator-=(const Fraction& rhs)
    {
        value_ -= rhs.value_;
        return *this;
    }
    constexpr Fraction& operator+=(int rhs)
    {
        value_ += static_cast<int64_t>(rhs) * Precision;
        return *this;
    }
    constexpr Fraction& operator-=(int rhs)
    {
        value_ -= static_cast<int64_t>(rhs) * Precision;
        return *this;
    }
    constexpr Fraction& operator*=(int rhs)
    {
        value_ *= rhs;
        return *this;
    }

    constexpr Fraction operator-() const
    {
        return Fraction::from_raw(-value_);
    }

    // Comparison operators
    constexpr bool operator==(const Fraction& rhs) const
    {
        return value_ == rhs.value_;
    }
    constexpr bool operator!=(const Fraction& rhs) const
    {
        return value_ != rhs.value_;
    }
    constexpr bool operator<(const Fraction& rhs) const
    {
        return value_ < rhs.value_;
    }
    constexpr bool operator<=(const Fraction& rhs) const
    {
        return value_ <= rhs.value_;
    }
    constexpr bool operator>(const Fraction& rhs) const
    {
        return value_ > rhs.value_;
    }
    constexpr bool operator>=(const Fraction& rhs) const
    {
        return value_ >= rhs.value_;
    }

    // Static factory for raw value
    constexpr static Fraction from_raw(int64_t raw)
    {
        Fraction f;
        f.value_ = raw;
        return f;
    }

    template <int64_t NewPrecision>
    [[nodiscard]] constexpr Fraction<NewPrecision> rescale() const
    {
        if constexpr (NewPrecision < precision())
        {
            return Fraction<NewPrecision>::from_raw(raw() / (precision() / NewPrecision));
        }
        else
        {
            return Fraction<NewPrecision>::from_raw(raw() * (NewPrecision / precision()));
        }
    }
};

// Arithmetic operators between Fractions of different precisions
// Result precision is LCM to avoid any loss
template <int64_t P1, int64_t P2>
constexpr auto operator+(const Fraction<P1>& lhs, const Fraction<P2>& rhs) -> Fraction<std::lcm(P1, P2)>
{
    constexpr int64_t ResultPrecision = std::lcm(P1, P2);
    constexpr int64_t scale1 = ResultPrecision / P1;
    constexpr int64_t scale2 = ResultPrecision / P2;
    return Fraction<ResultPrecision>::from_raw(lhs.raw() * scale1 + rhs.raw() * scale2);
}

template <int64_t P1, int64_t P2>
constexpr auto operator-(const Fraction<P1>& lhs, const Fraction<P2>& rhs) -> Fraction<std::lcm(P1, P2)>
{
    constexpr int64_t ResultPrecision = std::lcm(P1, P2);
    constexpr int64_t scale1 = ResultPrecision / P1;
    constexpr int64_t scale2 = ResultPrecision / P2;
    return Fraction<ResultPrecision>::from_raw(lhs.raw() * scale1 - rhs.raw() * scale2);
}

// Multiplication between fractions (handles same and different precisions)
template <int64_t P1, int64_t P2>
constexpr auto operator*(const Fraction<P1>& lhs, const Fraction<P2>& rhs) -> Fraction<P1 * P2>
{
    // (lhs.raw / P1) * (rhs.raw / P2) = (lhs.raw * rhs.raw) / (P1 * P2)
    return Fraction<P1 * P2>::from_raw(lhs.raw() * rhs.raw());
}

// Comparison operators between different precisions
template <int64_t P1, int64_t P2>
constexpr bool operator==(const Fraction<P1>& lhs, const Fraction<P2>& rhs)
{
    constexpr int64_t CommonPrecision = std::lcm(P1, P2);
    constexpr int64_t scale1 = CommonPrecision / P1;
    constexpr int64_t scale2 = CommonPrecision / P2;
    return lhs.raw() * scale1 == rhs.raw() * scale2;
}

template <int64_t P1, int64_t P2>
constexpr bool operator!=(const Fraction<P1>& lhs, const Fraction<P2>& rhs)
{
    return !(lhs == rhs);
}

template <int64_t P1, int64_t P2>
constexpr bool operator<(const Fraction<P1>& lhs, const Fraction<P2>& rhs)
{
    constexpr int64_t CommonPrecision = std::lcm(P1, P2);
    constexpr int64_t scale1 = CommonPrecision / P1;
    constexpr int64_t scale2 = CommonPrecision / P2;
    return lhs.raw() * scale1 < rhs.raw() * scale2;
}

template <int64_t P1, int64_t P2>
constexpr bool operator<=(const Fraction<P1>& lhs, const Fraction<P2>& rhs)
{
    return !(rhs < lhs);
}

template <int64_t P1, int64_t P2>
constexpr bool operator>(const Fraction<P1>& lhs, const Fraction<P2>& rhs)
{
    return rhs < lhs;
}

template <int64_t P1, int64_t P2>
constexpr bool operator>=(const Fraction<P1>& lhs, const Fraction<P2>& rhs)
{
    return !(lhs < rhs);
}

// Free function operators for int on left side
template <int64_t Precision>
constexpr Fraction<Precision> operator+(int lhs, const Fraction<Precision>& rhs)
{
    return rhs + lhs;
}

template <int64_t Precision>
constexpr Fraction<Precision> operator-(int lhs, const Fraction<Precision>& rhs)
{
    return Fraction<Precision>::from_raw(static_cast<int64_t>(lhs) * Precision - rhs.raw());
}

template <int64_t Precision>
constexpr Fraction<Precision> operator*(int lhs, const Fraction<Precision>& rhs)
{
    return rhs * lhs;
}

// Comparison operators with int
template <int64_t Precision>
constexpr bool operator==(const Fraction<Precision>& lhs, int rhs)
{
    return lhs.raw() == static_cast<int64_t>(rhs) * Precision;
}

template <int64_t Precision>
constexpr bool operator==(int lhs, const Fraction<Precision>& rhs)
{
    return rhs == lhs;
}

template <int64_t Precision>
constexpr bool operator!=(const Fraction<Precision>& lhs, int rhs)
{
    return !(lhs == rhs);
}

template <int64_t Precision>
constexpr bool operator!=(int lhs, const Fraction<Precision>& rhs)
{
    return !(rhs == lhs);
}

template <int64_t Precision>
constexpr bool operator<(const Fraction<Precision>& lhs, int rhs)
{
    return lhs.raw() < static_cast<int64_t>(rhs) * Precision;
}

template <int64_t Precision>
constexpr bool operator<(int lhs, const Fraction<Precision>& rhs)
{
    return static_cast<int64_t>(lhs) * Precision < rhs.raw();
}

template <int64_t Precision>
constexpr bool operator<=(const Fraction<Precision>& lhs, int rhs)
{
    return lhs.raw() <= static_cast<int64_t>(rhs) * Precision;
}

template <int64_t Precision>
constexpr bool operator<=(int lhs, const Fraction<Precision>& rhs)
{
    return static_cast<int64_t>(lhs) * Precision <= rhs.raw();
}

template <int64_t Precision>
constexpr bool operator>(const Fraction<Precision>& lhs, int rhs)
{
    return lhs.raw() > static_cast<int64_t>(rhs) * Precision;
}

template <int64_t Precision>
constexpr bool operator>(int lhs, const Fraction<Precision>& rhs)
{
    return static_cast<int64_t>(lhs) * Precision > rhs.raw();
}

template <int64_t Precision>
constexpr bool operator>=(const Fraction<Precision>& lhs, int rhs)
{
    return lhs.raw() >= static_cast<int64_t>(rhs) * Precision;
}

template <int64_t Precision>
constexpr bool operator>=(int lhs, const Fraction<Precision>& rhs)
{
    return static_cast<int64_t>(lhs) * Precision >= rhs.raw();
}

// Stream output operator
template <int64_t Precision>
inline std::ostream& operator<<(std::ostream& os, const Fraction<Precision>& f)
{
    return os << (double)f.raw() / Precision << " (" << f.raw() << "/" << Precision << ")";
}

template <int64_t Precision>
constexpr Fraction<Precision> abs(const Fraction<Precision>& f)
{
    return f.raw() >= 0 ? f : -f;
}

template <int64_t NewPrecision, int64_t OldPrecision>
constexpr Fraction<NewPrecision> rescale(const Fraction<OldPrecision>& f)
{
    return f.template rescale<NewPrecision>();
}

// Compile-time tests
namespace fraction_tests
{
// Basic construction and conversion
static_assert(Fraction<256>(0).raw() == 0);
static_assert(Fraction<256>(1).raw() == 256);
static_assert(Fraction<256>(5).raw() == 1280);
static_assert(Fraction<256>(5).to_int() == 5);

// Addition with same precision
static_assert((Fraction<256>(2) + Fraction<256>(3)).raw() == 1280);
static_assert((Fraction<256>(2) + Fraction<256>(3)).to_int() == 5);

// Addition with int
static_assert((Fraction<256>(2) + 3).raw() == 1280);
static_assert((3 + Fraction<256>(2)).raw() == 1280);

// Subtraction with same precision
static_assert((Fraction<256>(5) - Fraction<256>(2)).raw() == 768);
static_assert((Fraction<256>(5) - Fraction<256>(2)).to_int() == 3);

// Subtraction with int
static_assert((Fraction<256>(5) - 2).raw() == 768);
static_assert((5 - Fraction<256>(2)).raw() == 768);

// Multiplication with same precision (7/4 * 9/4 = 63/16)
static_assert((Fraction<4>::from_raw(7) * Fraction<4>::from_raw(9)).raw() == 63);
static_assert((Fraction<4>::from_raw(7) * Fraction<4>::from_raw(9)).precision() == 16);
static_assert((Fraction<4>::from_raw(7) * Fraction<4>::from_raw(9)).to_int() == 3);

// Multiplication with int
static_assert((Fraction<256>(2) * 3).raw() == 1536);
static_assert((3 * Fraction<256>(2)).raw() == 1536);

// Comparison operators
static_assert(Fraction<256>(5) == Fraction<256>(5));
static_assert(Fraction<256>(5) != Fraction<256>(3));
static_assert(Fraction<256>(3) < Fraction<256>(5));
static_assert(Fraction<256>(5) > Fraction<256>(3));
static_assert(Fraction<256>(5) <= Fraction<256>(5));
static_assert(Fraction<256>(5) >= Fraction<256>(5));

// Comparison with int
static_assert(Fraction<256>(5) == 5);
static_assert(5 == Fraction<256>(5));
static_assert(Fraction<256>(3) < 5);
static_assert(3 < Fraction<256>(5));

// Mixed precision addition
static_assert((Fraction<4>(2) + Fraction<8>(1)).raw() == 24); // 2 + 1 = 3, precision lcm(4,8)=8
static_assert((Fraction<4>(2) + Fraction<8>(1)).precision() == 8);
static_assert((Fraction<4>(2) + Fraction<8>(1)).to_int() == 3);

// Mixed precision subtraction
static_assert((Fraction<4>(3) + Fraction<8>(2)).raw() == 40); // 3 + 2 = 5, precision 8
static_assert((Fraction<4>(3) + Fraction<8>(2)).precision() == 8);
static_assert((Fraction<4>(3) + Fraction<8>(2)).to_int() == 5);

// Mixed precision multiplication (7/4 * 9/8 = 63/32)
static_assert((Fraction<4>::from_raw(7) * Fraction<8>::from_raw(9)).raw() == 63);
static_assert((Fraction<4>::from_raw(7) * Fraction<8>::from_raw(9)).precision() == 32);
static_assert((Fraction<4>::from_raw(7) * Fraction<8>::from_raw(9)).to_int() == 1);

// Negation
static_assert((-Fraction<256>(5)).raw() == -1280);

// Absolute value
static_assert(abs(Fraction<256>(5)).raw() == 1280);
static_assert(abs(Fraction<256>(-5)).raw() == 1280);

// Compound assignment
constexpr auto test_compound_add = []()
{
    Fraction<256> f(2);
    f += Fraction<256>(3);
    return f.raw();
};
static_assert(test_compound_add() == 1280);

constexpr auto test_compound_mul = []()
{
    Fraction<256> f(2);
    f *= 3;
    return f.raw();
};
static_assert(test_compound_mul() == 1536);

static_assert(rescale<512>(Fraction<256>::from_raw(123)).raw() == 246); // 123/256 = 246/512
static_assert(rescale<256>(Fraction<512>::from_raw(123)).raw() == 61); // 123/512 = 61/256
}