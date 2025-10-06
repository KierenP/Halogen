#pragma once
#include <cmath>
#include <cstdint>
#include <type_traits>

template <int Precision>
class Fraction
{
    static_assert(Precision > 0, "Precision must be positive");
    int64_t value_; // stores the value as an integer multiple of 1/Precision

public:
    // Constructors
    Fraction()
        : value_(0)
    {
    }

    Fraction(int v)
        : value_(static_cast<int64_t>(v) * Precision)
    {
    }

    // Copy/move
    Fraction(const Fraction&) = default;
    Fraction(Fraction&&) = default;
    Fraction& operator=(const Fraction&) = default;
    Fraction& operator=(Fraction&&) = default;

    int to_int() const
    {
        return value_ / Precision;
    }

    // Get raw value
    int64_t raw() const
    {
        return value_;
    }

    // Arithmetic operators
    Fraction operator+(const Fraction& rhs) const
    {
        return Fraction::from_raw(value_ + rhs.value_);
    }
    Fraction operator-(const Fraction& rhs) const
    {
        return Fraction::from_raw(value_ - rhs.value_);
    }
    Fraction operator*(int rhs) const
    {
        return Fraction::from_raw(value_ * rhs);
    }
    Fraction operator/(int rhs) const
    {
        return Fraction::from_raw(value_ / rhs);
    }

    Fraction& operator+=(const Fraction& rhs)
    {
        value_ += rhs.value_;
        return *this;
    }
    Fraction& operator-=(const Fraction& rhs)
    {
        value_ -= rhs.value_;
        return *this;
    }
    Fraction& operator*=(int rhs)
    {
        value_ = value_ * rhs;
        return *this;
    }
    Fraction& operator/=(int rhs)
    {
        value_ = value_ / rhs;
        return *this;
    }
    Fraction operator-() const
    {
        return Fraction::from_raw(-value_);
    }

    // Comparison operators
    bool operator==(const Fraction& rhs) const
    {
        return value_ == rhs.value_;
    }
    bool operator!=(const Fraction& rhs) const
    {
        return value_ != rhs.value_;
    }
    bool operator<(const Fraction& rhs) const
    {
        return value_ < rhs.value_;
    }
    bool operator<=(const Fraction& rhs) const
    {
        return value_ <= rhs.value_;
    }
    bool operator>(const Fraction& rhs) const
    {
        return value_ > rhs.value_;
    }
    bool operator>=(const Fraction& rhs) const
    {
        return value_ >= rhs.value_;
    }

    // Static factory for raw value
    static Fraction from_raw(int64_t raw)
    {
        Fraction f;
        f.value_ = raw;
        return f;
    }
};