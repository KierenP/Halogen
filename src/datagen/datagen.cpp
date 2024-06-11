#include "datagen.h"

#include "../EvalNet.h"
#include "../MoveGeneration.h"
#include "../Search.h"
#include "../SearchData.h"
#include "../uci/uci.h"
#include "encode.h"

#include <atomic>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>

void self_play_game(Uci& uci, std::ofstream& data_file);

std::atomic<uint64_t> games;
std::atomic<uint64_t> fens;
std::atomic<uint64_t> white_wins;
std::atomic<uint64_t> draws;
std::atomic<uint64_t> black_wins;

bool stop = false;

void info_thread()
{
    using namespace std::chrono_literals;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point last_print = start;

    uint64_t last_games = games;
    uint64_t last_fens = fens;
    uint64_t last_white_wins = white_wins;
    uint64_t last_draws = draws;
    uint64_t last_black_wins = black_wins;

    while (!stop)
    {
        // wake up once a second and print diagnostics if needed.
        // by sleeping, we avoid spinning endlessly and contesting shared memory.
        std::this_thread::sleep_for(1s);

        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

        if (now - last_print >= 10s)
        {
            auto duration = std::chrono::duration<float>(now - last_print).count();

            std::cout << "Games " << games << " (" << int(float(games - last_games) / duration) << "/s)" << std::endl;
            std::cout << "Fens " << fens << " (" << int(float(fens - last_fens) / duration) << "/s)" << std::endl;
            std::cout << "WDL %: ";
            std::cout << float(white_wins - last_white_wins) / float(games - last_games) << " ";
            std::cout << float(draws - last_draws) / float(games - last_games) << " ";
            std::cout << float(black_wins - last_black_wins) / float(games - last_games);
            std::cout << std::endl;

            last_print = std::chrono::steady_clock::now();
            last_games = games;
            last_fens = fens;
            last_white_wins = white_wins;
            last_draws = draws;
            last_black_wins = black_wins;
        }

        if (now - start >= 30s)
        {
            stop = true;
        }
    }
}

void generation_thread(Uci& uci, std::string_view output_path)
{
    uci.shared.limits.nodes = 40000;
    uci.output_level = OutputLevel::None;
    tTable.SetSize(1);
    std::ofstream data_file(output_path.data(), std::ios::out | std::ios::binary | std::ios::app);

    while (!stop)
    {
        self_play_game(uci, data_file);
        uci.handle_ucinewgame();
    }
}

void datagen(Uci& uci, std::string_view output_path)
{
    auto info = std::thread(info_thread);
    auto data = std::thread(generation_thread, std::ref(uci), output_path);

    info.join();
    data.join();
}

void self_play_game(Uci& uci, std::ofstream& data_file)
{
    auto& position = uci.position;
    auto& data = uci.shared;
    thread_local std::mt19937 gen(0);

    const auto& opening = []() -> std::string
    {
        // starting position;
        return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    }();

    bool valid_opening = false;

    while (!valid_opening)
    {
        int turns = 0;
        uci.position.InitialiseFromFen(opening);

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
        if (std::abs(eval.value()) < 500)
        {
            valid_opening = true;
        }
    }

    std::vector<std::tuple<BoardState, int16_t, Move>> datapoints;
    float result = -1;

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
                if (position.Board().stm == WHITE)
                {
                    result = 0.f;
                    black_wins++;
                }
                else
                {
                    result = 1.f;
                    white_wins++;
                }
            }
            else
            {
                result = 0.5f;
                draws++;
            }

            break;
        }

        // 50 move rule
        if (position.Board().fifty_move_count >= 100)
        {
            result = 0.5f;
            draws++;
            break;
        }

        // 3 fold repitition rule
        if (position.CheckForRep(0, 3))
        {
            result = 0.5f;
            draws++;
            break;
        }

        // insufficent material rule
        if (DeadPosition(position.Board()))
        {
            result = 0.5f;
            draws++;
            break;
        }

        // -----------------------------

        SearchThread(position, data);
        auto search_result = data.get_best_search_result();
        datapoints.emplace_back(position.Board(), search_result.score.value(), search_result.best_move);
        position.ApplyMove(search_result.best_move);
    }

    for (const auto& [board, eval, best_move] : datapoints)
    {
        auto data_point = convert(board, eval, result, best_move);
        data_file.write(reinterpret_cast<const char*>(&data_point), sizeof(data_point));
    }

    games++;
    fens += datapoints.size();
}
