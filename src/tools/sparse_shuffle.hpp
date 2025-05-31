#pragma once

#include "nn/Arch.hpp"
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

        std::array<bool, FT_SIZE / 2> stm;
        std::array<bool, FT_SIZE / 2> nstm;

        for (size_t i = 0; i < FT_SIZE / 2; i++)
        {
            stm[i] = ft_activation[i] > 0;
            nstm[i] = ft_activation[i + FT_SIZE / 2] > 0;
        }

        activation.push_back(stm);
        activation.push_back(nstm);
    }

    constexpr static size_t group_size = 4;
    constexpr static size_t num_groups = (FT_SIZE / 2) / group_size;
    static_assert(group_size * num_groups == (FT_SIZE / 2));

    void GroupNeuronsByCoactivation()
    {
        std::vector<bool> used(FT_SIZE / 2, false);

        std::vector<std::vector<int>> groups;

        for (size_t i = 0; i < num_groups; i++)
        {
            auto& group = groups.emplace_back();
            for (size_t j = 0; j < group_size; j++)
            {
                group.push_back(i * 4 + j);
            }
        }

        RefineGroups(groups);
        PrintCurrentBest(groups);
    }

private:
    int64_t GroupScore(const std::vector<int>& group)
    {
        // Count the number of times this group would result in a zero block.
        int64_t score = 0;
#pragma omp parallel for reduction(+ : score)
        for (size_t i = 0; i < activation.size(); i++)
        {
            if (!activation[i][group[0]] && !activation[i][group[1]] && !activation[i][group[2]]
                && !activation[i][group[3]])
            {
                score++;
            }
        }
        return score;
    }

    void EstimateNNZ(std::vector<std::vector<int>>& groups)
    {
        int64_t total = num_groups * activation.size();
        int64_t nnz = 0;

#pragma omp parallel for reduction(+ : nnz)
        for (size_t i = 0; i < activation.size(); i++)
        {
            for (size_t j = 0; j < groups.size(); j++)
            {
                if (activation[i][groups[j][0]] || activation[i][groups[j][1]] || activation[i][groups[j][2]]
                    || activation[i][groups[j][3]])
                {
                    nnz++;
                }
            }
        }

        std::cout << "Estimated nnz: " << float(nnz) / float(total) * 100 << "%" << std::endl;
    }

    void RefineGroups(std::vector<std::vector<int>>& groups)
    {
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

        std::cout << "Finished refining groups" << std::endl;
    }

    void PrintCurrentBest(std::vector<std::vector<int>>& groups)
    {
        std::cout << "[";
        for (const auto& group : groups)
            for (const auto& val : group)
                std::cout << val << ", ";
        std::cout << "]" << std::endl;
    }

    std::vector<std::array<bool, FT_SIZE / 2>> activation = {};
};

#ifdef NETWORK_SHUFFLE
inline SparseL1Shuffle shuffle_network_data;
inline size_t block_count = 0;
inline size_t block_nnz = 0;
inline size_t neuron_count = 0;
inline size_t neuron_nnz = 0;
#endif