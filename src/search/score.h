#pragma once

#include "bitboard/define.h"

#include <cstdint>
#include <cstdlib>
#include <ostream>

class Score
{
public:
    Score() = default;

    constexpr Score(int value) noexcept
        : value_(value)
    {
    }

    struct Limits
    {
        static constexpr int MATED = -10000;
        static constexpr int MATE = 10000;

        static constexpr int TB_LOSS_SCORE = MATED + MAX_RECURSION + 1;
        static constexpr int TB_WIN_SCORE = MATE - MAX_RECURSION - 1;

        static constexpr int EVAL_MIN = TB_LOSS_SCORE + MAX_RECURSION + 1;
        static constexpr int EVAL_MAX = TB_WIN_SCORE - MAX_RECURSION - 1;

        static constexpr int DRAW = 0;
    };

    // helper factory functions
    static constexpr Score mated_in(int depth) noexcept
    {
        return Score(Limits::MATED + depth);
    }

    static constexpr Score mate_in(int depth) noexcept
    {
        return Score(Limits::MATE - depth);
    }

    static constexpr Score tb_loss_in(int depth) noexcept
    {
        return Score(Limits::TB_LOSS_SCORE + depth);
    }

    static constexpr Score tb_win_in(int depth) noexcept
    {
        return Score(Limits::TB_WIN_SCORE - depth);
    }

    static constexpr Score draw() noexcept
    {
        return Score(Limits::DRAW);
    }

    // Draw randomness as in https://github.com/Luecx/Koivisto/commit/c8f01211c290a582b69e4299400b667a7731a9f7
    static constexpr Score draw_random(uint64_t x) noexcept
    {
        return 8 - (x & 0b1111);
    }

    constexpr int value() const noexcept
    {
        return value_;
    }

    // unary minus
    constexpr Score operator-() const noexcept
    {
        return Score(-value_);
    }

    // comparison operators
    constexpr bool operator<(const Score& rhs) const noexcept
    {
        return value_ < rhs.value_;
    }

    constexpr bool operator>(const Score& rhs) const noexcept
    {
        return value_ > rhs.value_;
    }

    constexpr bool operator<=(const Score& rhs) const noexcept
    {
        return value_ <= rhs.value_;
    }

    constexpr bool operator>=(const Score& rhs) const noexcept
    {
        return value_ >= rhs.value_;
    }

    constexpr bool operator==(const Score& rhs) const noexcept
    {
        return value_ == rhs.value_;
    }

    constexpr bool operator!=(const Score& rhs) const noexcept
    {
        return value_ != rhs.value_;
    }

    constexpr Score& operator+=(const Score& rhs) noexcept
    {
        value_ += rhs.value_;
        return *this;
    }

    friend constexpr Score operator+(const Score& lhs, const Score& rhs) noexcept
    {
        return Score(lhs.value_ + rhs.value_);
    }

    constexpr Score& operator-=(const Score& rhs) noexcept
    {
        value_ -= rhs.value_;
        return *this;
    }

    friend constexpr Score operator-(const Score& lhs, const Score& rhs) noexcept
    {
        return Score(lhs.value_ - rhs.value_);
    }

    // scalar multiplication
    constexpr Score& operator*=(const int& value) noexcept
    {
        value_ *= value;
        return *this;
    }

    friend constexpr Score operator*(const Score& lhs, const int& value) noexcept
    {
        return Score(lhs.value_ * value);
    }

    // scalar division
    constexpr Score& operator/=(const int& value) noexcept
    {
        value_ /= value;
        return *this;
    }

    friend constexpr Score operator/(const Score& lhs, const int& value) noexcept
    {
        return Score(lhs.value_ / value);
    }

    // print formatting
    friend std::ostream& operator<<(std::ostream& os, const Score& s)
    {
        return os << s.value() << "cp";
    }

    // helper functions
    [[nodiscard]] constexpr bool is_win() const noexcept
    {
        return *this >= Score::tb_win_in(MAX_RECURSION);
    }

    [[nodiscard]] constexpr bool is_draw() const noexcept
    {
        return *this == Score::draw();
    }

    [[nodiscard]] constexpr bool is_loss() const noexcept
    {
        return *this <= Score::tb_loss_in(MAX_RECURSION);
    }

    [[nodiscard]] constexpr bool is_decisive() const noexcept
    {
        return is_win() || is_loss();
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

inline Score abs(Score val) noexcept
{
    return std::abs(val.value());
}
}