#include "datagen.h"

#include "chessboard/game_state.h"
#include "encode.h"
#include "evaluation/evaluate.h"
#include "movegen/movegen.h"
#include "search/data.h"
#include "search/limit/limits.h"
#include "search/search.h"
#include "search/thread.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>

void self_play_game(GameState& position, SearchThreadPool& pool, const SearchLimits& limits, std::ofstream& data_file);

std::atomic<uint64_t> games;
std::atomic<uint64_t> fens;
std::atomic<uint64_t> white_wins;
std::atomic<uint64_t> draws;
std::atomic<uint64_t> black_wins;

bool stop = false;

void info_thread(std::chrono::seconds datagen_time)
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

        if ((now - start) >= datagen_time)
        {
            stop = true;
        }
    }
}

void generation_thread(std::string_view output_path)
{
    UCI::UciOutput output;
    SearchThreadPool pool { output, 1 };
    pool.set_hash(1);

    SearchLimits limits;
    limits.nodes = 40000;

    GameState position = GameState::starting_position();

    int output_rotation = 0;
    auto output_file_path = [&] { return std::string(output_path) + std::to_string(output_rotation) + ".data"; };
    std::ofstream data_file(output_file_path(), std::ios::out | std::ios::binary | std::ios::app);
    auto last_rotation = std::chrono::steady_clock::now();

    while (!stop)
    {
        position = GameState::starting_position();
        pool.reset_new_game();
        self_play_game(position, pool, limits, data_file);

        // rotate output file each hour
        auto now = std::chrono::steady_clock::now();
        if ((now - last_rotation) >= std::chrono::hours(1))
        {
            last_rotation = std::chrono::steady_clock::now();
            output_rotation++;
            data_file = std::ofstream(output_file_path(), std::ios::out | std::ios::binary | std::ios::app);
        }
    }
}

void datagen(std::string_view output_path, std::chrono::seconds duration)
{
    auto info = std::thread(info_thread, duration);
    auto data = std::thread(generation_thread, output_path);

    info.join();
    data.join();
}

void self_play_game(GameState& position, SearchThreadPool& pool, const SearchLimits& limits, std::ofstream& data_file)
{
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());

    const auto& opening = []() -> std::string
    {
        // starting position;
        return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    }();

    bool valid_opening = false;

    while (!valid_opening)
    {
        int turns = 0;
        position = GameState::from_fen(opening);

        // play out 10 random moves
        while (turns < 10)
        {
            BasicMoveList moves;
            legal_moves(position.board(), moves);

            if (moves.size() == 0)
            {
                // checkmate -> reset and generate a new opening line
                break;
            }

            std::uniform_int_distribution<> move(0, moves.size() - 1);
            position.apply_move(moves[move(gen)]);
            turns++;
            continue;
        }

        BasicMoveList moves;
        legal_moves(position.board(), moves);

        if (moves.size() == 0)
        {
            // checkmate -> reset and generate a new opening line
            continue;
        }

        pool.set_position(position);
        const auto& search_result = pool.launch_search(limits);
        auto eval = search_result.score;

        // check static eval is within set margin
        if (std::abs(eval.value()) < 500)
        {
            valid_opening = true;
        }
    }

    BoardState initial_state = position.board();
    std::vector<std::tuple<int16_t, Move>> score_moves;
    float result = -1;

    while (true)
    {
        // check for a terminal position

        // checkmate or stalemate
        BasicMoveList moves;
        legal_moves(position.board(), moves);
        if (moves.size() == 0)
        {
            if (is_in_check(position.board()))
            {
                if (position.board().stm == WHITE)
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
        if (position.board().fifty_move_count >= 100)
        {
            result = 0.5f;
            draws++;
            break;
        }

        // 3 fold repitition rule
        if (position.is_repetition(0))
        {
            result = 0.5f;
            draws++;
            break;
        }

        // insufficient material rule
        if (insufficient_material(position.board()))
        {
            result = 0.5f;
            draws++;
            break;
        }

        // -----------------------------

        pool.set_position(position);
        const auto& search_result = pool.launch_search(limits);
        const auto best_move = search_result.pv[0];
        const auto white_relative_score = (position.board().stm == WHITE ? 1 : -1) * search_result.score.value();
        score_moves.emplace_back(white_relative_score, best_move);
        position.apply_move(best_move);
    }

    auto marlin_format = convert(initial_state, result);
    data_file.write(reinterpret_cast<const char*>(&marlin_format), sizeof(marlin_format));
    auto stm = initial_state.stm;

    for (const auto& [eval, best_move] : score_moves)
    {
        auto packed_move = convert(best_move);
        auto score = convert(stm, eval);
        data_file.write(reinterpret_cast<const char*>(&packed_move), sizeof(packed_move));
        data_file.write(reinterpret_cast<const char*>(&score), sizeof(score));
        stm = !stm;
    }

    // terminate the game
    uint8_t terminator[4] = { 0, 0, 0, 0 };
    data_file.write(reinterpret_cast<const char*>(terminator), sizeof(terminator));

    games++;
    fens += score_moves.size();
}
