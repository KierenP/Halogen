#pragma once
#include <array>
#include <random>

// 12 pieces * 64 squares, 1 for side to move, 4 for casteling rights and 8 for ep. square

namespace Zobrist
{
	constexpr size_t SIDE = 12 * 64;
	constexpr size_t CASTLE_K = 12 * 64 + 1;
	constexpr size_t CASTLE_Q = 12 * 64 + 2;
	constexpr size_t CASTLE_k = 12 * 64 + 3;
	constexpr size_t CASTLE_q = 12 * 64 + 4;
	constexpr size_t EN_PASSANT = 12 * 64 + 5;

	constexpr size_t SIZE = 12 * 64 + 1 + 4 + 8;

	inline auto Init()
	{
		std::array<uint64_t, SIZE> ret;

		std::mt19937_64 gen(0);
		std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

		for (auto& val : ret)
		{
			val = dist(gen);
		}

		return ret;
	}

	static std::array<uint64_t, SIZE> Table = Init();
}