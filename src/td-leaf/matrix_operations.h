#include <algorithm>
#include <array>
#include <cstdlib>

template <typename OUT, typename IN, size_t SIZE>
[[nodiscard]] OUT dot_product(const std::array<IN, SIZE>& lhs, const std::array<IN, SIZE>& rhs)
{
    OUT result = 0;
    for (size_t i = 0; i < SIZE; i++)
    {
        result += lhs[i] * rhs[i];
    }
    return result;
}

template <typename OUT, typename IN, size_t SIZE>
[[nodiscard]] OUT dot_product_halves(
    const std::array<IN, SIZE>& stm, const std::array<IN, SIZE>& other, const std::array<IN, SIZE * 2>& weights)
{
    OUT result = 0;
    for (size_t i = 0; i < SIZE; i++)
    {
        result += stm[i] * weights[i];
    }

    for (size_t i = 0; i < SIZE; i++)
    {
        result += other[i] * weights[i + SIZE];
    }
    return result;
}

template <typename T, size_t SIZE>
void apply_ReLU(std::array<T, SIZE>& source)
{
    for (size_t i = 0; i < SIZE; i++)
        source[i] = std::max(T(0), source[i]);
}

template <typename T, size_t SIZE>
[[nodiscard]] std::array<T, SIZE> copy_ReLU(const std::array<T, SIZE>& source)
{
    std::array<T, SIZE> ret;

    for (size_t i = 0; i < SIZE; i++)
        ret[i] = std::max(T(0), source[i]);

    return ret;
}