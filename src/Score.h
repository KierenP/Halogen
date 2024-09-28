#pragma once

#include <cstdint>
#include <limits>
#include <ostream>

class Score
{
public:
    Score() = default;

    constexpr Score(int value)
        : value_(value)
    {
    }

    struct Limits
    {
        static constexpr int MATED = -10000;
        static constexpr int MATE = 10000;

        static constexpr int TB_LOSS_SCORE = -9900;
        static constexpr int TB_WIN_SCORE = 9900;

        static constexpr int EVAL_MIN = -9800;
        static constexpr int EVAL_MAX = 9800;

        static constexpr int DRAW = 0;
    };

    // helper factory functions
    static constexpr Score mated_in(int depth)
    {
        return Score(Limits::MATED + depth);
    }

    static constexpr Score mate_in(int depth)
    {
        return Score(Limits::MATE - depth);
    }

    static constexpr Score tb_loss_in(int depth)
    {
        return Score(Limits::TB_LOSS_SCORE + depth);
    }

    static constexpr Score tb_win_in(int depth)
    {
        return Score(Limits::TB_WIN_SCORE - depth);
    }

    static constexpr Score draw()
    {
        return Score(Limits::DRAW);
    }

    constexpr int value() const
    {
        return value_;
    }

    // unary minus
    constexpr Score operator-() const
    {
        return Score(-value_);
    }

    // comparison operators
    constexpr bool operator<(const Score& rhs) const
    {
        return value_ < rhs.value_;
    }

    constexpr bool operator>(const Score& rhs) const
    {
        return value_ > rhs.value_;
    }

    constexpr bool operator<=(const Score& rhs) const
    {
        return value_ <= rhs.value_;
    }

    constexpr bool operator>=(const Score& rhs) const
    {
        return value_ >= rhs.value_;
    }

    constexpr bool operator==(const Score& rhs) const
    {
        return value_ == rhs.value_;
    }

    constexpr bool operator!=(const Score& rhs) const
    {
        return value_ != rhs.value_;
    }

    constexpr Score& operator+=(const Score& rhs)
    {
        value_ += rhs.value_;
        return *this;
    }

    friend constexpr Score operator+(const Score& lhs, const Score& rhs)
    {
        return Score(lhs.value_ + rhs.value_);
    }

    constexpr Score& operator-=(const Score& rhs)
    {
        value_ -= rhs.value_;
        return *this;
    }

    friend constexpr Score operator-(const Score& lhs, const Score& rhs)
    {
        return Score(lhs.value_ - rhs.value_);
    }

    // scalar multiplication
    constexpr Score& operator*=(const int& value)
    {
        value_ *= value;
        return *this;
    }

    friend constexpr Score operator*(const Score& lhs, const int& value)
    {
        return Score(lhs.value_ * value);
    }

    // scalar division
    constexpr Score& operator/=(const int& value)
    {
        value_ /= value;
        return *this;
    }

    friend constexpr Score operator/(const Score& lhs, const int& value)
    {
        return Score(lhs.value_ / value);
    }

    // print formatting
    friend std::ostream& operator<<(std::ostream& os, const Score& s)
    {
        return os << s.value() << "cp";
    }

private:
    int16_t value_;
};

static constexpr Score SCORE_UNDEFINED = -32768;

namespace std
{
template <>
class numeric_limits<Score>
{
public:
    static constexpr Score min() noexcept
    {
        return Score(-30000);
    };

    static constexpr Score max() noexcept
    {
        return Score(30000);
    };
};

inline Score abs(Score val)
{
    return std::abs(val.value());
}
}