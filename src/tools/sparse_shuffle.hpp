#pragma once

#include "nn/Arch.hpp"
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

// Scheme inspired by the Integral chess engine
class SparseL1Shuffle
{
public:
    void report_ft_activations(const std::array<uint8_t, FT_SIZE>& ft_activation)
    {
        // We need to remember we do a pairwise mul (so each activation refers to a pair of ft neurons), we have a duel
        // perspective net (so each ft_activation array is really 2 observations for each ft neuron). We also ensure we
        // are using sse4 mode so that we don't need to adjust for the packus shuffling.

        for (size_t i = 0; i < FT_SIZE; i++)
        {
            if (!ft_activation[i])
            {
                continue;
            }

            activation[i]++;

            for (size_t j = 0; j < FT_SIZE; ++j)
            {
                if (!ft_activation[j])
                {
                    continue;
                }

                coactivation[i % (FT_SIZE / 2)][j % (FT_SIZE / 2)]++;
            }
        }
    }

    std::vector<int> GroupNeuronsByCoactivation()
    {
        constexpr static size_t group_size = 4;
        constexpr static size_t num_groups = (FT_SIZE / 2) / group_size;
        static_assert(group_size * num_groups == (FT_SIZE / 2));
        std::vector<bool> used(FT_SIZE / 2, false);
        std::vector<std::vector<int>> groups;

        std::cout << "Creating groups..." << std::endl;

        for (size_t g = 0; g < num_groups; ++g)
        {
            std::vector<int> group;

            // greedly pick the next most active neuron to form a group
            std::optional<int> seed;
            int64_t max_sum = std::numeric_limits<int64_t>::min();

            for (size_t i = 0; i < FT_SIZE / 2; i++)
            {
                if (used[i])
                {
                    continue;
                }

                if (activation[i] > max_sum)
                {
                    max_sum = activation[i];
                    seed = i;
                }
            }

            used[*seed] = true;
            group.push_back(*seed);

            // greedly pick remaining neurons to maximize coactivation with current group
            for (size_t k = 0; k < group_size - 1; ++k)
            {
                std::optional<int> best;
                int64_t best_score = std::numeric_limits<int64_t>::min();
                for (size_t j = 0; j < FT_SIZE / 2; ++j)
                {
                    if (used[j])
                    {
                        continue;
                    }
                    int score = 0;
                    for (int m : group)
                        score += coactivation[m][j];
                    if (score > best_score)
                    {
                        best_score = score;
                        best = j;
                    }
                }

                used[*best] = true;
                group.push_back(*best);
            }

            groups.push_back(std::move(group));
        }

        std::cout << "Finished creating groups" << std::endl;

        RefineGroups(groups);

        // Flatten the refined groups into a sorted neuron list
        std::vector<int> result;
        for (const auto& group : groups)
            result.insert(result.end(), group.begin(), group.end());

        return result;
    }

private:
    int64_t GroupScore(const std::vector<int>& group)
    {
        int64_t score = 0;
        for (size_t i = 0; i < group.size(); ++i)
        {
            for (size_t j = i + 1; j < group.size(); ++j)
            {
                score += coactivation[group[i]][group[j]];
            }
        }
        return score;
    }

    void RefineGroups(std::vector<std::vector<int>>& groups)
    {
        std::cout << "Refining groups..." << std::endl;

        bool improved = true;
        while (improved)
        {
            improved = false;

            // try two-opt swaps to greedly improve overall score
            for (size_t g1 = 0; g1 < groups.size(); g1++)
            {
                for (size_t g2 = g1 + 1; g2 < groups.size(); g2++)
                {
                    for (size_t i = 0; i < 4; i++)
                    {
                        for (size_t j = 0; j < 4; j++)
                        {
                            auto g1_copy = groups[g1];
                            auto g2_copy = groups[g2];
                            std::swap(g1_copy[i], g2_copy[j]);

                            int64_t old_score = GroupScore(groups[g1]) + GroupScore(groups[g2]);
                            int64_t new_score = GroupScore(g1_copy) + GroupScore(g2_copy);

                            if (new_score > old_score)
                            {
                                groups[g1] = std::move(g1_copy);
                                groups[g2] = std::move(g2_copy);
                                improved = true;
                            }
                        }
                    }
                }
            }
        }

        std::cout << "Finished refining groups" << std::endl;
    }

    std::array<int64_t, FT_SIZE / 2> activation = {};
    std::array<std::array<int64_t, FT_SIZE / 2>, FT_SIZE / 2> coactivation = {};
};

#ifdef NETWORK_SHUFFLE
inline SparseL1Shuffle shuffle_network_data;
inline size_t block_count = 0;
inline size_t block_nnz = 0;
inline size_t neuron_count = 0;
inline size_t neuron_nnz = 0;
#endif