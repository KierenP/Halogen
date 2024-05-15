#include "TrainableNetwork.h"
#include "../BoardState.h"
#include "HalogenNetwork.h"
#include "matrix_operations.h"
#include "training_values.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <tuple>
#include <type_traits>
#include <utility>

// apply the adam gradients to the weight layer
template <template <typename, size_t, size_t> class layer_t, typename T, size_t in, size_t out>
void apply_gradient(layer_t<T, in, out>& layer, layer_t<TrainableNetwork::adam_state, in, out>& adam,
    const layer_t<T, in, out>& gradient, int n_samples, uint64_t t);

// given a loss gradient of layer l, and an activation for layer l-1, update the adam states in layer l
template <typename T, size_t in, size_t out>
void update_gradient(
    Layer<T, in, out>& gradient, const std::array<T, out>& loss_gradient, const std::array<T, in>& activation);

template <typename T, size_t in, size_t out>
void update_gradient_sparse_half(TransposeLayer<T, in, out>& gradient, const std::array<T, out * 2>& loss_gradient,
    const std::vector<int>& sparse_stm, const std::vector<int>& sparse_other);

template <typename T, size_t in, size_t out, typename Activation_prime>
std::array<T, in> calculate_loss_gradient(Layer<T, in, out>& layer, const std::array<T, out>& gradient,
    const std::array<T, in>& activation, Activation_prime&& activation_prime);

decltype(TrainableNetwork::l1_adam) TrainableNetwork::l1_adam;
decltype(TrainableNetwork::l2_adam) TrainableNetwork::l2_adam;
std::atomic<uint64_t> TrainableNetwork::t = 0;

std::mutex TrainableNetwork::l1_lock;
std::mutex TrainableNetwork::l2_lock;

std::array<std::vector<int>, N_PLAYERS> TrainableNetwork::GetSparseInputs(const BoardState& position)
{
    // this should closely match the implementation of the HalogenNetwork::Recalculate() function

    std::array<std::vector<int>, N_PLAYERS> sparseInputs;
    sparseInputs[WHITE].reserve(32);
    sparseInputs[BLACK].reserve(32);

    for (int i = 0; i < N_PIECES; i++)
    {
        Pieces piece = static_cast<Pieces>(i);
        uint64_t bb = position.GetPieceBB(piece);

        while (bb)
        {
            Square sq = LSBpop(bb);
            sparseInputs[WHITE].push_back(index(sq, piece, WHITE));
            sparseInputs[BLACK].push_back(index(sq, piece, BLACK));
        }
    }

    return sparseInputs;
}

void TrainableNetwork::InitializeWeightsRandomly(bool print_diagnostics)
{
    std::scoped_lock lock { l1_lock, l2_lock };

    std::mt19937 gen(0);

    auto initialize = [&gen](auto&& layer)
    {
        // Kaiming He initialization
        std::normal_distribution<float> dis(0, sqrt(2.0 / layer.in_count_v));

        for (auto& row : layer.weight)
        {
            for (auto& element : row)
            {
                element = dis(gen);
            }
        }
    };

    initialize(l1);
    initialize(l2);

    if (print_diagnostics)
    {
        PrintNetworkDiagnostics();
    }
}

void TrainableNetwork::SaveWeights(const std::string& filename, bool print_diagnostics)
{
    std::scoped_lock lock { l1_lock, l2_lock };

    if (print_diagnostics)
    {
        PrintNetworkDiagnostics();
    }

    std::ofstream file(filename, std::ios::out | std::ios::binary);

    auto save_bias = [&file](auto& layer)
    {
        for (size_t i = 0; i < layer.bias.size(); i++)
        {
            file.write(reinterpret_cast<const char*>(&layer.bias[i]),
                sizeof(typename std::decay_t<decltype(layer)>::value_type));
        }
    };

    auto save_layer = [&file, &save_bias](auto& layer)
    {
        for (size_t i = 0; i < layer.out_count_v; i++)
        {
            for (size_t j = 0; j < layer.in_count_v; j++)
            {
                file.write(reinterpret_cast<const char*>(&layer.weight[i][j]),
                    sizeof(typename std::decay_t<decltype(layer)>::value_type));
            }
        }

        save_bias(layer);
    };

    auto save_transpose_layer = [&file, &save_bias](auto& layer)
    {
        for (size_t i = 0; i < layer.out_count_v; i++)
        {
            for (size_t j = 0; j < layer.in_count_v; j++)
            {
                file.write(reinterpret_cast<const char*>(&layer.weight[j][i]),
                    sizeof(typename std::decay_t<decltype(layer)>::value_type));
            }
        }

        save_bias(layer);
    };

    save_transpose_layer(l1);
    save_layer(l2);
}

void TrainableNetwork::UpdateGradients(
    double loss_gradient, const std::array<std::vector<int>, N_PLAYERS>& sparse_inputs, Players stm)
{
    // do the forward pass and save the activations:

    std::array<float, architecture[1] * 2> l1_activation;

    std::copy(l1.bias.begin(), l1.bias.end(), l1_activation.begin());
    std::copy(l1.bias.begin(), l1.bias.end(), l1_activation.begin() + architecture[1]);

    for (size_t i = 0; i < sparse_inputs[stm].size(); i++)
    {
        for (int j = 0; j < architecture[1]; j++)
        {
            l1_activation[j] += l1.weight[sparse_inputs[stm][i]][j];
        }
    }

    for (size_t i = 0; i < sparse_inputs[!stm].size(); i++)
    {
        for (int j = 0; j < architecture[1]; j++)
        {
            l1_activation[j + architecture[1]] += l1.weight[sparse_inputs[!stm][i]][j];
        }
    }

    apply_ReLU(l1_activation);

    // get loss gradients
    auto l2_loss_gradient = std::array { static_cast<float>(loss_gradient) };
    auto l1_loss_gradient
        = calculate_loss_gradient(l2, l2_loss_gradient, l1_activation, [](float x) { return x > 0.0f; });

    // gradient updates
    update_gradient_sparse_half(l1_gradient, l1_loss_gradient, sparse_inputs[stm], sparse_inputs[!stm]);
    update_gradient(l2_gradient, l2_loss_gradient, l1_activation);
}

void TrainableNetwork::ApplyOptimizationStep(int n_samples)
{
    t++;

    {
        std::scoped_lock lock { l1_lock };
        apply_gradient(l1, l1_adam, l1_gradient, n_samples, t);
    }
    {
        std::scoped_lock lock { l2_lock };
        apply_gradient(l2, l2_adam, l2_gradient, n_samples, t);
    }

    // zero out the gradients for the next iteration
    l1_gradient = {};
    l2_gradient = {};
}

template <template <typename, size_t, size_t> class layer_t, typename T, size_t in, size_t out>
void apply_gradient(layer_t<T, in, out>& layer, layer_t<TrainableNetwork::adam_state, in, out>& adam,
    const layer_t<T, in, out>& gradient, int n_samples, uint64_t t)
{
    // bias adjustment
    auto adj_alpha = lr_alpha * std::sqrt(1 - std::pow(TrainableNetwork::adam_state::beta_2, t))
        / (1 - std::pow(TrainableNetwork::adam_state::beta_1, t));

    for (size_t i = 0; i < layer.weight.size(); i++)
    {
        for (size_t j = 0; j < layer.weight[i].size(); j++)
        {
            // get the average gradient
            double g = gradient.weight[i][j] / n_samples;

            // do the adam step
            adam.weight[i][j].m = TrainableNetwork::adam_state::beta_1 * adam.weight[i][j].m
                + (1 - TrainableNetwork::adam_state::beta_1) * g;
            adam.weight[i][j].v = TrainableNetwork::adam_state::beta_2 * adam.weight[i][j].v
                + (1 - TrainableNetwork::adam_state::beta_2) * g * g;

            // update the weights
            layer.weight[i][j] += -adj_alpha * adam.weight[i][j].m
                / std::sqrt(adam.weight[i][j].v + TrainableNetwork::adam_state::epsilon);
        }
    }

    for (size_t i = 0; i < layer.bias.size(); i++)
    {
        // get the average gradient
        double g = gradient.bias[i] / n_samples;

        // do the adam step
        adam.bias[i].m
            = TrainableNetwork::adam_state::beta_1 * adam.bias[i].m + (1 - TrainableNetwork::adam_state::beta_1) * g;
        adam.bias[i].v = TrainableNetwork::adam_state::beta_2 * adam.bias[i].v
            + (1 - TrainableNetwork::adam_state::beta_2) * g * g;

        // update the weights
        layer.bias[i]
            += -adj_alpha * adam.bias[i].m / std::sqrt(adam.bias[i].v + TrainableNetwork::adam_state::epsilon);
    }
}

template <typename T, size_t in, size_t out>
void update_gradient(
    Layer<T, in, out>& gradient, const std::array<T, out>& loss_gradient, const std::array<T, in>& activation)
{
    for (size_t i = 0; i < gradient.weight.size(); i++)
    {
        for (size_t j = 0; j < gradient.weight[i].size(); j++)
        {
            double g = loss_gradient[i] * activation[j];
            gradient.weight[i][j] += g;
        }

        double g = loss_gradient[i];
        gradient.bias[i] += g;
    }
}

template <typename T, size_t in, size_t out>
void update_gradient_sparse_half(TransposeLayer<T, in, out>& gradient, const std::array<T, out * 2>& loss_gradient,
    const std::vector<int>& sparse_stm, const std::vector<int>& sparse_other)
{
    for (size_t i = 0; i < gradient.weight.size(); i++)
    {
        double activation_stm = std::find(sparse_stm.begin(), sparse_stm.end(), i) != sparse_stm.end() ? 1 : 0;
        double activation_other = std::find(sparse_other.begin(), sparse_other.end(), i) != sparse_other.end() ? 1 : 0;

        for (size_t j = 0; j < gradient.weight[i].size(); j++)
        {
            double g = loss_gradient[j] * activation_stm + loss_gradient[j + out] * activation_other;
            gradient.weight[i][j] += g;
        }
    }

    for (size_t i = 0; i < gradient.bias.size(); i++)
    {
        double g = loss_gradient[i] + loss_gradient[i + out];
        gradient.bias[i] += g;
    }
}

template <typename T, size_t in, size_t out, typename Activation_prime>
std::array<T, in> calculate_loss_gradient(Layer<T, in, out>& layer, const std::array<T, out>& gradient,
    const std::array<T, in>& activation, Activation_prime&& activation_prime)
{
    std::array<T, in> loss_gradient = {};

    for (size_t i = 0; i < layer.weight.size(); i++)
    {
        for (size_t j = 0; j < layer.weight[i].size(); j++)
        {
            loss_gradient[j] += gradient[i] * layer.weight[i][j];
        }
    }

    for (size_t i = 0; i < loss_gradient.size(); i++)
    {
        loss_gradient[i] *= activation_prime(activation[i]);
    }

    return loss_gradient;
}

void TrainableNetwork::PrintNetworkDiagnostics()
{
    std::scoped_lock lock { l1_lock, l2_lock };

    auto print_layer = [&](auto&& layer)
    {
        for (size_t i = 0; i < layer.weight.size(); i++)
        {
            for (size_t j = 0; j < layer.weight[i].size(); j++)
            {
                std::cout << layer.weight[i][j] << " ";
            }

            std::cout << std::endl;
        }

        std::cout << std::endl;

        for (size_t i = 0; i < layer.bias.size(); i++)
        {
            std::cout << layer.bias[i] << " ";
        }

        std::cout << std::endl;
        std::cout << std::endl;
    };

    print_layer(l1);
    print_layer(l2);
}

bool TrainableNetwork::VerifyWeightReadWrite()
{
    // Randomly initialize the weights, then write to a file, then read, and check we get back what we expected

    InitializeWeightsRandomly();

    auto l1_copy = l1;
    auto l2_copy = l2;

    SaveWeights("/tmp/verify_weights.nn");
    InitializeWeightsRandomly();
    LoadWeights("/tmp/verify_weights.nn");

    if (l1_copy == l1 && l2_copy == l2)
    {
        std::cout << "Verified reading/writing of network file\n";
        return true;
    }
    else
    {
        std::cout << "Error, verification of reading/writing network file failed\n";
        return false;
    }
}