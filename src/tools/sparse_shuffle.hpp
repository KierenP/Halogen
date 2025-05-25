#pragma once

#include "nn/Arch.hpp"
#include <iostream>
#include <vector>

class SparseL1Shuffle
{
public:
#if defined(USE_SSE4) and not defined(USE_AVX2)
    void report_ft_activations(const std::array<uint8_t, FT_SIZE>& ft_activation)
    {
        // We need to remember we do a pairwise mul (so each activation refers to a pair of ft neurons), we have a duel
        // perspective net (so each ft_activation array is really 2 observations for each ft neuron). We also ensure we
        // are using sse4 mode so that we don't need to adjust for the packus shuffling.

        total_count += 2;
        for (size_t i = 0; i < ft_activation.size(); i++)
        {
            if (ft_activation[i] > 0)
            {
                // account for duel perspective
                auto j = i % (FT_SIZE / 2);
                active_count[j]++;
            }
        }

        if (total_count % (1024 * 1024) == 0)
        {
            print_permutation();
        }
    }
#endif

    void print_permutation()
    {
        std::vector<std::pair<size_t, size_t>> count_idx;
        count_idx.reserve(active_count.size());
        for (size_t i = 0; i < active_count.size(); i++)
        {
            // adjust for pairwise mul
            count_idx.emplace_back(active_count[i], i);
        }

        std::sort(count_idx.begin(), count_idx.end(), std::greater<> {});

        std::cout << "Summary: \n";

        for (const auto& [count, idx] : count_idx)
        {
            std::cout << "Neuron: " << idx << " active: " << float(count) / float(total_count) * 100 << "%\n";
        }

        std::cout << "[";
        for (const auto& [_, idx] : count_idx)
        {
            std::cout << idx << ", ";
        }
        std::cout << "]\n";
    }

private:
    size_t total_count = 0;
    std::array<size_t, FT_SIZE / 2> active_count = {};
};