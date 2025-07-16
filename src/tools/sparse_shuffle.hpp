#pragma once

#include "network/arch.hpp"
#include <chrono>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

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
            activation[i % (FT_SIZE / 2)].push_back(ft_activation[i] > 0);
        }

        activation_data_count += 2;
    }

    constexpr static size_t group_size = 4;
    constexpr static size_t num_groups = (FT_SIZE / 2) / group_size;
    static_assert(group_size * num_groups == (FT_SIZE / 2));

    using Group = std::array<int, group_size>;
    using Groups = std::array<Group, num_groups>;

    void GroupNeuronsByCoactivation()
    {
        Groups groups;

        for (size_t i = 0; i < num_groups; i++)
        {
            for (size_t j = 0; j < group_size; j++)
            {
                groups[i][j] = i * 4 + j;
            }
        }

        RefineGroups(groups);
        PrintCurrentBest(groups);
        EstimateNNZ(groups);
    }

private:
    int64_t GroupScore(const Group& group)
    {
        // Count the number of times this group would result in a zero block.
        int64_t score = 0;
#pragma omp parallel for reduction(+ : score)
        for (size_t i = 0; i < activation_data_count; i++)
        {
            if (!activation[group[0]][i] && !activation[group[1]][i] && !activation[group[2]][i]
                && !activation[group[3]][i])
            {
                score++;
            }
        }
        return score;
    }

    void EstimateNNZ(const Groups& groups)
    {
        int64_t total = num_groups * activation_data_count;
        int64_t nnz = 0;

#pragma omp parallel for reduction(+ : nnz)
        for (size_t i = 0; i < activation_data_count; i++)
        {
            for (size_t j = 0; j < groups.size(); j++)
            {
                if (activation[groups[j][0]][i] || activation[groups[j][1]][i] || activation[groups[j][2]][i]
                    || activation[groups[j][3]][i])
                {
                    nnz++;
                }
            }
        }

        std::cout << "Estimated nnz: " << float(nnz) / float(total) * 100 << "%" << std::endl;
    }

    void RefineGroups(Groups& groups)
    {
        auto begin = std::chrono::high_resolution_clock::now();

        std::cout << "Refining groups..." << std::endl;

        auto UpdateBest = [&, last_update = std::chrono::high_resolution_clock::now()]() mutable
        {
            auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_update).count() > 10)
            {
                last_update = now;
                EstimateNNZ(groups);
                PrintCurrentBest(groups);
            }
        };

        bool improved = true;
        while (improved)
        {
            improved = false;

            // try two-opt swaps to greedly improve overall score
            for (size_t g1 = 0; g1 < groups.size(); g1++)
            {
                for (size_t g2 = g1 + 1; g2 < groups.size(); g2++)
                {
                    std::cout << "Trying to swap groups " << g1 << " and " << g2 << std::endl;
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
                                UpdateBest();
                            }
                        }
                    }
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
        std::cout << "Refinement took " << duration << " seconds" << std::endl;
        std::cout << "Finished refining groups" << std::endl;
    }

    void PrintCurrentBest(const Groups& groups)
    {
        std::cout << "[";
        for (const auto& group : groups)
            for (const auto& val : group)
                std::cout << val << ", ";
        std::cout << "]" << std::endl;
    }

    // faster to store std::vector<uint8_t> than std::vector<bool> for auto-vectorization
    std::array<std::vector<uint8_t>, FT_SIZE / 2> activation = {};
    size_t activation_data_count = 0;
};

#ifdef NETWORK_SHUFFLE
inline SparseL1Shuffle shuffle_network_data;
inline size_t block_count = 0;
inline size_t block_nnz = 0;
inline size_t neuron_count = 0;
inline size_t neuron_nnz = 0;
#endif