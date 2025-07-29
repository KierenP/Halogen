#pragma once

#include <chrono>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include "chessboard/game_state.h"
#include "search/data.h"

class Move;
class SearchThreadPool;

namespace UCI
{

class UciOutput;

enum class OutputLevel : uint8_t
{
    None,
    Minimal,
    Default,

    ENUM_END
};

class Uci
{
public:
    Uci(std::string_view version, SearchThreadPool& pool, UciOutput& output);
    ~Uci();

    Uci(const Uci&) = delete;
    Uci& operator=(const Uci&) = delete;
    Uci(Uci&&) = delete;
    Uci& operator=(Uci&&) = delete;

    void process_input_stream(std::istream& is);
    void process_input(std::string_view command);

    void print_search_info(const SearchSharedState& shared, const SearchResults& data, bool final = false);
    void print_bestmove(bool chess960, Move move);
    void print_error(const std::string& error_str);

    struct go_ctx
    {
        std::optional<std::chrono::milliseconds> wtime;
        std::optional<std::chrono::milliseconds> btime;
        std::optional<std::chrono::milliseconds> winc;
        std::optional<std::chrono::milliseconds> binc;
        std::optional<int> movestogo;
        std::optional<std::chrono::milliseconds> movetime;
        std::optional<int> mate;
        std::optional<int> depth;
        std::optional<int> nodes;
    };

    struct datagen_ctx
    {
        std::string output_path;
        std::chrono::seconds duration;
    };

    void handle_uci();
    void handle_isready();
    void handle_ucinewgame();
    bool handle_position_fen(std::string_view fen);
    void handle_moves(std::string_view move);
    void handle_position_startpos();
    void handle_go(go_ctx& ctx);
    void handle_setoption_clear_hash();
    void handle_setoption_hash(int value);
    void handle_setoption_threads(int value);
    void handle_setoption_syzygy_path(std::string_view value);
    void handle_setoption_multipv(int value);
    void handle_setoption_chess960(bool value);
    void handle_setoption_output_level(OutputLevel level);
    void handle_stop();
    void handle_quit();
    void handle_bench(int depth);
    void handle_spsa();
    void handle_print();
    void handle_eval();
    void handle_probe();
    void handle_datagen(const datagen_ctx& ctx);
    void handle_shuffle_network();
    void handle_analyse(std::string_view path);

private:
    void join_search_thread();

    const std::string_view version_;

    SearchThreadPool& search_thread_pool;
    UciOutput& output;
    std::thread main_search_thread;
    GameState position = GameState::starting_position();
    bool quit = false;
    bool finished_startup = false;

    auto options_handler();
};

// A handler that gets passed to the search for it to print info including bestmove
class UciOutput
{
public:
    OutputLevel output_level = OutputLevel::Default;

    void print_search_info(
        const SearchSharedState& shared, const SearchResults& data, const BoardState& board, bool final = false);
    void print_bestmove(bool chess960, Move move);
    void print_error(const std::string& error_str);
};

}