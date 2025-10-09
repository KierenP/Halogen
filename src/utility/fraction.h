#pragma once
#include <cmath>
#include <cstdint>
#include <ostream>

template <int Precision>
class Fraction
{
    static_assert(Precision > 0, "Precision must be positive");
    int64_t value_; // stores the value as an integer multiple of 1/Precision

public:
    // Constructors
    constexpr Fraction()
        : value_(0)
    {
    }

    constexpr explicit Fraction(int v)
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

    // Arithmetic operators
    constexpr Fraction operator+(const Fraction& rhs) const
    {
        return Fraction::from_raw(value_ + rhs.value_);
    }
    constexpr Fraction operator-(const Fraction& rhs) const
    {
        return Fraction::from_raw(value_ - rhs.value_);
    }
    constexpr Fraction operator*(int rhs) const
    {
        return Fraction::from_raw(value_ * rhs);
    }
    constexpr Fraction operator/(int rhs) const
    {
        return Fraction::from_raw(value_ / rhs);
    }

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
    constexpr Fraction& operator*=(int rhs)
    {
        value_ = value_ * rhs;
        return *this;
    }
    constexpr Fraction& operator/=(int rhs)
    {
        value_ = value_ / rhs;
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
};

// Stream output operator
template <int Precision>
inline std::ostream& operator<<(std::ostream& os, const Fraction<Precision>& f)
{
    return os << (double)f.raw() / Precision << " (" << f.raw() << "/" << Precision << ")";
}