#include "datagen.h"

#include "chessboard/game_state.h"
#include "encode.h"
#include "evaluation/evaluate.h"
#include "movegen/movegen.h"
#include "search/data.h"
#include "search/limit/limits.h"
#include "search/search.h"
#include "search/thread.h"
#include "uci/uci.h"

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

std::atomic<uint64_t> games_eligible_for_adjudication;
std::atomic<uint64_t> correct_adjudications;

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
            std::cout << "Adjudication Accuracy: "
                      << 100.f * float(correct_adjudications) / float(games_eligible_for_adjudication) << "%";
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
    UCI::UciOutput output { UCI::OutputLevel::None };
    SearchThreadPool pool { output, 1, 1, 1 };

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

    // Experimentally, it was found that applying adjudication to some portion of games (but not all) gives a better
    // distribution of data for training. The adjudication parameters below were chosen empirically, they result in
    // around a 40% reduction in average game length with around 99% adjudication accuracy.
    constexpr static int win_adjudication_score = 750;
    constexpr static int win_adjudication_plys = 6;
    constexpr static int draw_adjudication_score = 10;
    constexpr static int draw_adjudication_plys = 12;
    constexpr static double adjudication_fraction = 0.65;

    int white_win_adj_count = 0;
    int black_win_adj_count = 0;
    int draw_adj_count = 0;

    bool would_be_eligible_for_adjudication = false;
    float potential_adjudication_decision = -1;
    bool adjudicate_this_game = std::bernoulli_distribution(adjudication_fraction)(gen);

    while (true)
    {
        // check for a terminal position

        // checkmate or stalemate
        BasicMoveList moves;
        legal_moves(position.board(), moves);
        if (moves.size() == 0)
        {
            if (position.board().checkers)
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
        const auto score = search_result.score.value();
        score_moves.emplace_back(score, best_move);
        auto white_relative_score = (position.board().stm == WHITE ? score : -score);

        // perform win adjudication
        if (white_relative_score > win_adjudication_score)
        {
            white_win_adj_count++;
        }
        else
        {
            white_win_adj_count = 0;
        }

        if (white_relative_score < -win_adjudication_score)
        {
            black_win_adj_count++;
        }
        else
        {
            black_win_adj_count = 0;
        }

        // perform draw adjudication
        if (std::abs(white_relative_score) < draw_adjudication_score)
        {
            draw_adj_count++;
        }
        else
        {
            draw_adj_count = 0;
        }

        if (adjudicate_this_game)
        {
            if (white_win_adj_count >= win_adjudication_plys)
            {
                result = 1.f;
                white_wins++;
                break;
            }
            else if (black_win_adj_count >= win_adjudication_plys)
            {
                result = 0.f;
                black_wins++;
                break;
            }
            else if (draw_adj_count >= draw_adjudication_plys)
            {
                result = 0.5f;
                draws++;
                break;
            }
        }
        else if (!would_be_eligible_for_adjudication)
        {
            // even if we aren't adjudicating this game, this is a good opportunity to record stats on how often
            // adjudication would have occurred, and how often it would have been correct.
            if (white_win_adj_count >= win_adjudication_plys)
            {
                potential_adjudication_decision = 1.f;
                would_be_eligible_for_adjudication = true;
            }
            else if (black_win_adj_count >= win_adjudication_plys)
            {
                potential_adjudication_decision = 0.f;
                would_be_eligible_for_adjudication = true;
            }
            else if (draw_adj_count >= draw_adjudication_plys)
            {
                potential_adjudication_decision = 0.5f;
                would_be_eligible_for_adjudication = true;
            }
        }

        position.apply_move(best_move);
    }

    // track adjudication accuracy for summary stats
    if (would_be_eligible_for_adjudication)
    {
        games_eligible_for_adjudication++;
        if (potential_adjudication_decision == result)
        {
            correct_adjudications++;
        }
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
