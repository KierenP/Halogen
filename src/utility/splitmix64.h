#pragma once

#include <cstdint>

/*
 * SplitMix64 RNG for Zobrist hashing
 *
 * This implementation is based on Sebastiano Vigna's SplitMix64 (http://prng.di.unimi.it/).
 * It is a very fast, 64-bit pseudorandom number generator with strong bit diffusion.
 *
 * Why SplitMix64 is ideal for Zobrist hashing:
 * 1. High/low bit independence: Unlike mt19937_64, the high and low bits are well-mixed
 *    and independent. This is crucial because Zobrist hashing uses the high bits
 *    for table indexing and low bits for storing verification keys. Correlated bits
 *    could increase collisions in transposition tables.
 *
 * 2. Strong diffusion for XOR accumulation: Zobrist hashes are computed by XORing
 *    piece-square numbers. XOR is linear, so if the RNG has correlated bits, the
 *    resulting hash may lose entropy. SplitMix64's nonlinear mixing ensures all
 *    output bits are effectively independent, minimizing collisions.
 *
 * 3. Deterministic: Using a fixed seed produces a reproducible Zobrist table, which
 *    is important for testing, debugging, and consistent behavior across runs and
 *    machines.
 *
 * 4. Performance and simplicity: SplitMix64 is extremely fast and has a tiny state
 *    (just a 64-bit integer), making it ideal for generating all the random numbers
 *    needed for a chess engine's Zobrist table.
 *
 * 5. License: SplitMix64 is public domain
 *
 * Usage:
 *    SplitMix64 rng(seed);
 *    uint64_t rand64 = rng.next(); // generate next 64-bit random number
 *
 */

struct SplitMix64
{
    uint64_t state;

    SplitMix64(uint64_t seed) noexcept
        : state(seed)
    {
    }

    uint64_t next() noexcept
    {
        uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
};