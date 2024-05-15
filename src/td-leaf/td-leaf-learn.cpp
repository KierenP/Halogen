#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <mutex>
#include <random>
#include <string>
#include <thread>

#include "../EvalNet.h"
#include "../GameState.h"
#include "../MoveGeneration.h"
#include "../Search.h"
#include "../SearchData.h"
#include "HalogenNetwork.h"
#include "TrainableNetwork.h"
#include "td-leaf-learn.h"
#include "training_values.h"

void SelfPlayGame(TrainableNetwork& network, SearchSharedState& data, const std::vector<std::string>& openings);
void PrintNetworkDiagnostics(TrainableNetwork& network);

std::string weight_file_name(int epoch, int game);
std::vector<std::string> parse_opening_book(std::string_view path);

std::atomic<uint64_t> game_count = 0;

std::atomic<uint64_t> move_count = 0;
std::atomic<uint64_t> depth_count = 0;

std::atomic<bool> stop_signal = false;

void learn_thread(const std::vector<std::string>& openings)
{
    TrainableNetwork network;
    SearchSharedState data(1);
    data.silent_mode = true;
    data.limits.SetNodeLimit(training_nodes);

    while (!stop_signal)
    {
        SelfPlayGame(network, data, openings);
        game_count++;
    }
}

void info_thread(TrainableNetwork& network, int epoch)
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point last_print = start;
    std::chrono::steady_clock::time_point last_save = start;
    uint64_t game_count_last = 0;

    while (true)
    {
        // wake up once a second and print diagnostics if needed.
        // by sleeping, we avoid spinning endlessly and contesting shared memory.
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_print).count() >= 10)
        {
            auto duration = std::chrono::duration<float>(now - last_print).count();
            lr_alpha = learning_rate_schedule(
                std::chrono::duration<float>(now - start).count() / (60.0 * 60.0 * training_time_hours));
            td_lambda = prediction_quality_record.fit_psi();

            prediction_quality_record.debug_print();

            std::cout << "Game " << game_count << std::endl;
            std::cout << "Games per second: " << (game_count - game_count_last) / duration << std::endl;
            std::cout << "Average search depth: " << static_cast<double>(depth_count) / static_cast<double>(move_count)
                      << std::endl;
            std::cout << "Learning rate: " << lr_alpha << std::endl;
            std::cout << "Lambda: " << td_lambda << std::endl;
            std::cout << std::endl;
            std::cout << std::endl;

            last_print = std::chrono::steady_clock::now();
            game_count_last = game_count;
            move_count = 0;
            depth_count = 0;
            prediction_quality_record.clear();
        }

        if (std::chrono::duration_cast<std::chrono::minutes>(now - last_save).count() >= 15)
        {
            last_save = std::chrono::steady_clock::now();
            network.SaveWeights(weight_file_name(epoch, game_count));
        }

        if (std::chrono::duration<float>(now - start).count() >= 60.0 * 60.0 * training_time_hours)
        {
            std::cout << "Training complete." << std::endl;
            stop_signal = true;
            network.SaveWeights(weight_file_name(epoch, game_count));
            return;
        }
    }
}

void learn(const std::string& initial_weights_file, int epoch, const std::string& opening_book)
{
    if (!TrainableNetwork::VerifyWeightReadWrite())
    {
        return;
    }

    TrainableNetwork network;

    if (initial_weights_file == "none")
    {
        std::cout << "Initializing weights randomly\n";
        network.InitializeWeightsRandomly();
    }
    else
    {
        std::cout << "Initializing weights from file\n";
        network.LoadWeights(initial_weights_file);
    }

    // Save the random weights as a baseline
    network.SaveWeights(weight_file_name(epoch, 0));

    // Read in the opening book
    auto openings = parse_opening_book(opening_book);

    std::vector<std::thread> threads;

    threads.emplace_back(info_thread, std::ref(network), epoch);

    // always have at least one learning and one info thread.
    // at most we want max_threads total threads.
    for (int i = 0; i < std::max(1, max_threads - 1); i++)
    {
        threads.emplace_back([&]() { learn_thread(openings); });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
}

float sigmoid(float x)
{
    return 1.0f / (1.0f + exp(sigmoid_coeff * -x));
}

struct TD_game_result
{
    float score;
    Players stm;
    std::array<std::vector<int>, N_PLAYERS> sparseInputs = {};
    double delta = 0; // between this and next
};

void SelfPlayGame(TrainableNetwork& network, SearchSharedState& data, const std::vector<std::string>& openings)
{
    // std::chrono::steady_clock::time_point fn_begin = std::chrono::steady_clock::now();
    // uint64_t time_spend_in_search_ns = 0;

    thread_local std::mt19937 gen(0);

    GameState position;
    std::vector<TD_game_result> results;

    const auto& opening = [&openings]() -> std::string
    {
        // pick a random opening, or simply use the starting position
        std::uniform_real_distribution<> opening_prob(0, 1);

        if (opening_prob(gen) < opening_book_usage_pct)
        {
            std::uniform_int_distribution<> opening_line(0, openings.size() - 1);
            return openings[opening_line(gen)];
        }
        else
        {
            // starting position;
            return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        }
    }();

    bool valid_opening = false;

    while (!valid_opening)
    {
        int turns = 0;
        position.InitialiseFromFen(opening);

        // play out 10 random moves
        while (turns < 10)
        {
            BasicMoveList moves;
            LegalMoves(position.Board(), moves);

            if (moves.size() == 0)
            {
                // checkmate -> reset and generate a new opening line
                break;
            }

            std::uniform_int_distribution<> move(0, moves.size() - 1);
            position.ApplyMove(moves[move(gen)]);
            turns++;
            continue;
        }

        BasicMoveList moves;
        LegalMoves(position.Board(), moves);

        if (moves.size() == 0)
        {
            // checkmate -> reset and generate a new opening line
            continue;
        }

        SearchThread(position, data);
        auto result = data.get_best_search_result();
        auto eval = result.score;

        // check static eval is within set margin
        if (-opening_cutoff < eval && eval < opening_cutoff)
        {
            valid_opening = true;
        }
    }

    while (true)
    {
        // check for a terminal position

        // checkmate or stalemate
        BasicMoveList moves;
        LegalMoves(position.Board(), moves);
        if (moves.size() == 0)
        {
            if (IsInCheck(position.Board()))
            {
                results.push_back({ position.Board().stm == WHITE ? 0.f : 1.f, position.Board().stm });
            }
            else
            {
                results.push_back({ 0.5f, position.Board().stm });
            }

            break;
        }

        // 50 move rule
        if (position.Board().fifty_move_count >= 100)
        {
            results.push_back({ 0.5f, position.Board().stm });
            break;
        }

        // 3 fold repitition rule
        if (position.CheckForRep(0, 3))
        {
            results.push_back({ 0.5f, position.Board().stm });
            break;
        }

        // insufficent material rule
        if (DeadPosition(position.Board()))
        {
            results.push_back({ 0.5f, position.Board().stm });
            break;
        }

        // -----------------------------

        // std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        SearchThread(position, data);
        auto result = data.get_best_search_result();
        // std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        // time_spend_in_search_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
        depth_count += result.depth;
        move_count++;

        const auto& pv = result.pv;

        for (size_t i = 0; i < pv.size(); i++)
        {
            position.ApplyMove(pv[i]);
        }

        /*
        You could arguably use either the eval of the end of the pv or the score returned by the search.
        In theory for basic alpha-beta search these would be equal. That is not the case in practice due to
        search instability (transposition table etc) and search score adjustments (checkmate, draw randomness etc).
        */

        // adjust the relative scores from the net into a score from white's perspective
        auto pv_eval = position.GetEvaluation().value();
        pv_eval = position.Board().stm == WHITE ? pv_eval : -pv_eval;

        results.push_back({ sigmoid(pv_eval), position.Board().stm, network.GetSparseInputs(position.Board()) });

        for (size_t i = 0; i < pv.size(); i++)
        {
            position.RevertMove();
        }

        position.ApplyMove(result.best_move);
    }

    if (results.size() <= 1)
    {
        return;
    }

    for (size_t i = 0; i < results.size() - 1; i++)
    {
        results[i].delta = GAMMA * results[i + 1].score - results[i].score;
        // std::cout << "difference: " << results[i].difference << " scores: " << results[i].score << ", " << results[i
        // + 1].score << std::endl;
    }

    // main td-leaf update step:
    // we accumulate the gradients over the whole game and then apply the optimization step once
    // this ensures we get an equal proportion of learning from all games rather than more weight on long games.
    // It also means the batches are quite large, so learning rate might need to be adjusted

    for (size_t t = 0; t < results.size(); t++)
    {
        double delta_sum = 0;

        for (size_t j = t; j < results.size(); j++)
        {
            delta_sum += results[j].delta * pow(td_lambda * GAMMA, j - t);

            // contribute this td observation to the record
            prediction_quality_record.add_observation(j - t, results[t].score, results[j].score);
        }

        // note derivative of sigmoid with coefficent k is k*(s)*(1-s)
        double loss_gradient = -delta_sum * results[t].score * (1 - results[t].score) * sigmoid_coeff;

        // network outputs relative values, but temporal difference is from white's POV
        loss_gradient = results[t].stm == WHITE ? loss_gradient : -loss_gradient;

        network.UpdateGradients(loss_gradient, results[t].sparseInputs, results[t].stm);
    }

    network.ApplyOptimizationStep(results.size());

    // std::cout << "Game result: " << results.back().score << " turns: " << turns << std::endl;

    // std::chrono::steady_clock::time_point fn_end = std::chrono::steady_clock::now();
    // auto total_time = std::chrono::duration_cast<std::chrono::nanoseconds>(fn_end - fn_begin).count();
    // std::cout << "Time spend in search: " << static_cast<double>(time_spend_in_search_ns) /
    // static_cast<double>(total_time) * 100 << "%" << std::endl;
}

std::string weight_file_name(int epoch, int game)
{
    // format: 768-512x2-1_r123_g1234567890.nn"
    return "768-" + std::to_string(architecture[1]) + "x2-1_r" + std::to_string(epoch) + "_g" + std::to_string(game)
        + ".nn";
}

std::vector<std::string> parse_opening_book(std::string_view path)
{
    std::ifstream opening_book_file(path.data());

    if (!opening_book_file)
    {
        std::cout << "Error reading opening book file" << std::endl;
        return {};
    }

    std::string line;
    std::vector<std::string> lines;

    while (std::getline(opening_book_file, line))
    {
        lines.emplace_back(std::move(line));
    }

    std::cout << "Read " << lines.size() << " opening lines from file " << path << std::endl;

    return lines;
}