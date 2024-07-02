#pragma once

#include "../GameState.h"
#include "../SearchData.h"

#include <chrono>
#include <string_view>
#include <thread>

enum class OutputLevel
{
    None,
    Minimal,
    Default,

    ENUM_END
};

class Uci
{
public:
    Uci(std::string_view version);
    ~Uci();

    void process_input_stream(std::istream& is);
    void process_input(std::string_view command);

    void print_search_info(const SearchResults& data, bool final = false);
    void print_bestmove(Move move);

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

    struct syzygy_rescore_ctx
    {
        std::string input_path;
        std::string output_path;
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
    void handle_datagen(std::string_view path);
    void handle_syzygy_rescore(const syzygy_rescore_ctx& ctx);

    // private:
    void join_search_thread();

    GameState position;
    std::thread searchThread;
    SearchSharedState shared { *this };
    const std::string_view version_;
    OutputLevel output_level;
    bool quit = false;
    bool finished_startup = false;

    auto options_handler();
};
