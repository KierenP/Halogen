#pragma once

#include "../GameState.h"
#include "../SearchData.h"

#include <string_view>
#include <thread>

class Uci
{
public:
    Uci(std::string_view version);
    ~Uci();

    void process_input(std::string_view command);

    void print_search_info(const SearchResults& data);
    void print_bestmove(Move move);

    bool quit = false;

private:
    struct go_ctx
    {
        int wtime = 0;
        int btime = 0;
        int winc = 0;
        int binc = 0;
        int movestogo = 0;
        int movetime = 0;
    };

    void handle_uci();
    void handle_isready();
    void handle_ucinewgame();
    void handle_go(go_ctx& ctx);
    void handle_setoption_clear_hash();
    bool handle_setoption_hash(int value);
    void handle_setoption_threads(int value);
    void handle_setoption_syzygy_path(std::string_view value);
    void handle_setoption_multipv(int value);
    void handle_setoption_chess960(bool value);
    void handle_stop();
    void handle_quit();
    void handle_bench(int depth);

    void join_search_thread();

    GameState position;
    std::thread searchThread;
    SearchSharedState shared { *this };
    const std::string_view version_;

    auto options_handler();
};
