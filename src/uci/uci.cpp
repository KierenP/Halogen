#include "uci.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <ratio>
#include <sstream>
#include <string_view>
#include <vector>

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "chessboard/game_state.h"
#include "datagen/datagen.h"
#include "misc/benchmark.h"
#include "movegen/list.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "network/network.h"
#include "search/data.h"
#include "search/history.h"
#include "search/limit/limits.h"
#include "search/limit/time.h"
#include "search/score.h"
#include "search/search.h"
#include "search/syzygy.h"
#include "search/transposition/table.h"
#include "spsa/tuneable.h"
#include "uci/options.h"
#include "uci/parse.h"
#include "utility/atomic.h"
#include "utility/static_vector.h"

namespace UCI
{

std::ostream& operator<<(std::ostream& os, const OutputLevel& level)
{
    switch (level)
    {
    case OutputLevel::None:
        return os << "None";
    case OutputLevel::Minimal:
        return os << "Minimal";
    case OutputLevel::Default:
        return os << "Default";
    case OutputLevel::ENUM_END:
        return os;
    }

    assert(false);
    __builtin_unreachable();
}

template <>
std::optional<OutputLevel> to_enum<OutputLevel>(std::string_view str)
{
    if (str == "None")
    {
        return OutputLevel::None;
    }
    else if (str == "Minimal")
    {
        return OutputLevel::Minimal;
    }
    else if (str == "Default")
    {
        return OutputLevel::Default;
    }
    else
    {
        return std::nullopt;
    }
}

uint64_t Perft(int depth, GameState& position, bool check_legality)
{
    if (depth == 0)
        return 1; // if perftdivide is called with 1 this is necesary

    uint64_t nodeCount = 0;
    BasicMoveList moves;
    legal_moves(position.board(), moves);

    if (check_legality)
    {
        for (int i = 0; i < std::numeric_limits<uint16_t>::max(); i++)
        {
            Move move(i);
            bool legal = is_legal(position.board(), move);

            bool present = std::find(moves.begin(), moves.end(), move) != moves.end();

            if (present != legal)
            {
                std::cout << position.board() << move << "\n";
                std::cout << present << " " << legal << "\n";
                std::cout << move.from() << " " << move.to() << " " << move.flag() << std::endl;
                return 0; // cause perft answer to be incorrect
            }
        }
    }

    if (depth == 1)
        return moves.size();

    for (size_t i = 0; i < moves.size(); i++)
    {
        position.apply_move(moves[i]);
        nodeCount += Perft(depth - 1, position, check_legality);
        position.revert_move();
    }

    return nodeCount;
}

void PerftSuite(std::string path, int depth_reduce, bool check_legality)
{
    std::ifstream infile(path);

    int Perfts = 0;
    int Correct = 0;
    double Totalnodes = 0;
    std::string line;

    auto before = std::chrono::steady_clock::now();
    while (getline(infile, line))
    {
        std::vector<std::string> arrayTokens;
        std::istringstream iss(line);
        arrayTokens.clear();

        do
        {
            std::string stub;
            iss >> stub;
            arrayTokens.push_back(stub);
        } while (iss);

        std::string fen = arrayTokens[0] + " " + arrayTokens[1] + " " + arrayTokens[2] + " " + arrayTokens[3] + " "
            + arrayTokens[4] + " " + arrayTokens[5];

        auto position = GameState::from_fen(fen);

        int depth = (arrayTokens.size() - 7) / 2 - depth_reduce;
        uint64_t nodes = Perft(depth, position, check_legality);
        uint64_t correct = stoull(arrayTokens.at(arrayTokens.size() - 2 * (1 + depth_reduce)));
        if (nodes == stoull(arrayTokens.at(arrayTokens.size() - 2 * (1 + depth_reduce))))
        {
            std::cout << "CORRECT   (" << nodes << " == " << correct << ") [" << fen << "] depth: " << depth
                      << std::endl;
            Correct++;
        }
        else
        {
            std::cout << "INCORRECT (" << nodes << " != " << correct << ") [" << fen << "] depth: " << depth
                      << std::endl;
        }

        Totalnodes += nodes;
        Perfts++;
    }
    auto after = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(after - before).count();

    std::cout << "\n\nCompleted perft with: " << Correct << "/" << Perfts << " correct";
    std::cout << "\nTotal nodes: " << (Totalnodes) << " in " << duration << "s";
    std::cout << "\nNodes per second: " << static_cast<int>(Totalnodes / duration);
    std::cout << std::endl;
}

uint64_t PerftDivide(int depth, GameState& position, bool check_legality)
{
    auto before = std::chrono::steady_clock::now();

    uint64_t nodeCount = 0;
    BasicMoveList moves;
    legal_moves(position.board(), moves);

    for (size_t i = 0; i < moves.size(); i++)
    {
        position.apply_move(moves[i]);
        uint64_t ChildNodeCount = Perft(depth - 1, position, check_legality);
        position.revert_move();
        std::cout << moves[i] << ": " << ChildNodeCount << std::endl;
        nodeCount += ChildNodeCount;
    }

    auto after = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(after - before).count();

    std::cout << "\nNodes searched: " << (nodeCount) << " in " << duration << " seconds ";
    std::cout << "(" << static_cast<int>(nodeCount / duration) << " nps)" << std::endl;
    return nodeCount;
}

void Uci::handle_bench(int depth)
{
    Timer timer;

    uint64_t nodeCount = 0;
    shared.limits.depth = depth;

    for (size_t i = 0; i < benchMarkPositions.size(); i++)
    {
        if (!position.init_from_fen(benchMarkPositions[i]))
        {
            std::cout << "BAD FEN!" << std::endl;
            break;
        }

        shared.limits.time.reset();
        launch_search(position, shared);
        nodeCount += shared.nodes();
    }

    int elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(timer.elapsed()).count();
    std::cout << nodeCount << " nodes " << nodeCount / std::max(elapsed_time, 1) * 1000 << " nps" << std::endl;
}

auto Uci::options_handler()
{
#define tuneable_int(name, min_, max_)                                                                                 \
    SpinOption                                                                                                         \
    {                                                                                                                  \
        #name, name, min_, max_, [](auto value) { name = value; }                                                      \
    }

#define tuneable_float(name, min_, max_)                                                                               \
    FloatOption                                                                                                        \
    {                                                                                                                  \
        #name, name, min_, max_, [](auto value) { name = value; }                                                      \
    }

    return Options {
        ButtonOption { "Clear Hash", [this] { handle_setoption_clear_hash(); } },
        CheckOption { "UCI_Chess960", false, [this](bool value) { handle_setoption_chess960(value); } },
        SpinOption { "Hash", 32, 1, 262144, [this](auto value) { handle_setoption_hash(value); } },
        SpinOption { "Threads", 1, 1, 1024, [this](auto value) { handle_setoption_threads(value); } },
        SpinOption { "MultiPV", 1, 1, 256, [this](auto value) { handle_setoption_multipv(value); } },
        StringOption { "SyzygyPath", "<empty>", [this](auto value) { handle_setoption_syzygy_path(value); } },
        ComboOption {
            "OutputLevel", OutputLevel::Default, [this](auto value) { handle_setoption_output_level(value); } },

#ifdef TUNE
        tuneable_float(LMR_constant, -5, 5),
        tuneable_float(LMR_depth_coeff, -5, 5),
        tuneable_float(LMR_move_coeff, -5, 5),
        tuneable_float(LMR_depth_move_coeff, -1, 1),

        tuneable_int(aspiration_window_size, 5, 15),

        tuneable_int(nmp_const, 2, 6),
        tuneable_int(nmp_d, 4, 8),
        tuneable_int(nmp_s, 200, 300),
        tuneable_int(nmp_sd, 1, 10),

        tuneable_int(se_d, 32, 128),
        tuneable_int(se_de, 0, 30),

        tuneable_int(lmr_pv, 0, 2000),
        tuneable_int(lmr_cut, 0, 2000),
        tuneable_int(lmr_improving, -2000, 2000),
        tuneable_int(lmr_loud, -2000, 2000),
        tuneable_int(lmr_h, 1000, 5000),
        tuneable_int(lmr_offset, 0, 2000),

        tuneable_int(fifty_mr_scale_a, 100, 350),
        tuneable_int(fifty_mr_scale_b, 100, 350),

        tuneable_int(rfp_max_d, 5, 15),
        tuneable_int(rfp_d, 50, 150),

        tuneable_int(lmp_max_d, 3, 9),
        tuneable_int(lmp_const, 1, 10),
        tuneable_int(lmp_depth, 1, 10),

        tuneable_int(fp_max_d, 5, 15),
        tuneable_int(fp_const, 10, 50),
        tuneable_int(fp_depth, 5, 15),
        tuneable_int(fp_quad, 5, 25),

        tuneable_int(see_depth_quiet, 50, 150),
        tuneable_int(see_hist_quiet, 50, 250),
        tuneable_int(see_depth_loud, 10, 50),
        tuneable_int(see_hist_loud, 50, 250),

        tuneable_int(se_max_d, 5, 10),
        tuneable_int(se_tt_d, 0, 10),
        tuneable_int(se_pv, 0, 1000),
        tuneable_int(se_high_d, 0, 1000),
        tuneable_int(se_quiet, 0, 1000),

        tuneable_int(delta_c, 0, 500),

        tuneable_int(eval_scale_knight, 200, 800),
        tuneable_int(eval_scale_bishop, 200, 800),
        tuneable_int(eval_scale_rook, 400, 1200),
        tuneable_int(eval_scale_queen, 600, 2400),
        tuneable_int(eval_scale_const, 15000, 40000),

        tuneable_int(see_values[PAWN], 50, 200),
        tuneable_int(see_values[KNIGHT], 250, 1000),
        tuneable_int(see_values[BISHOP], 250, 1000),
        tuneable_int(see_values[ROOK], 300, 1400),
        tuneable_int(see_values[QUEEN], 600, 2400),

        tuneable_float(soft_tm, 0.1, 0.9),
        tuneable_float(node_tm_base, 0.2, 0.8),
        tuneable_float(node_tm_scale, 1.0, 3.0),
        tuneable_int(blitz_tc_a, 20, 80),
        tuneable_int(blitz_tc_b, 600, 2400),
        tuneable_int(sudden_death_tc, 25, 100),
        tuneable_int(repeating_tc, 50, 200),

        tuneable_int(PawnHistory::max_value, 5000, 16000),
        tuneable_int(PawnHistory::scale, 20, 50),
        tuneable_int(ThreatHistory::max_value, 5000, 16000),
        tuneable_int(ThreatHistory::scale, 20, 50),
        tuneable_int(CaptureHistory::max_value, 5000, 25000),
        tuneable_int(CaptureHistory::scale, 20, 60),
        tuneable_int(PawnHistory::max_value, 5000, 16000),
        tuneable_int(PawnHistory::scale, 20, 50),
        tuneable_int(PieceMoveHistory::max_value, 5000, 16000),
        tuneable_int(PieceMoveHistory::scale, 20, 50),

        tuneable_int(PawnCorrHistory::correction_max, 32, 128),
        tuneable_int(NonPawnCorrHistory::correction_max, 32, 128),

        tuneable_int(corr_hist_scale, 64, 256),
#else
#endif
    };

#undef tuneable_int
#undef tuneable_float
}

Uci::Uci(std::string_view version)
    : version_(version)
{
    options_handler().set_defaults();
    finished_startup = true;
}

Uci::~Uci()
{
    join_search_thread();
}

void Uci::handle_uci()
{
    std::cout << "id name Halogen " << version_ << "\n";
    std::cout << "id author Kieren Pearson\n";
    std::cout << options_handler();
    std::cout << "uciok" << std::endl;
}

void Uci::handle_isready()
{
    std::cout << "readyok" << std::endl;
}

void Uci::handle_ucinewgame()
{
    position = GameState::starting_position();
    shared.transposition_table.clear(shared.get_threads_setting());
    shared.reset_new_game();
}

bool Uci::handle_position_fen(std::string_view fen)
{
    return position.init_from_fen(fen);
}

void Uci::handle_moves(std::string_view move)
{
    position.apply_move(move);
}

void Uci::handle_position_startpos()
{
    position = GameState::starting_position();
}

void Uci::handle_go(go_ctx& ctx)
{
    shared.limits.mate = ctx.mate;
    shared.limits.depth = ctx.depth;
    shared.limits.nodes = ctx.nodes;
    shared.limits.time = {};

    using namespace std::chrono_literals;

    // The amount of time we leave on the clock for safety
    constexpr static auto BufferTime = 100ms;

    const auto& myTime = position.board().stm ? ctx.wtime : ctx.btime;
    const auto& myInc = position.board().stm ? ctx.winc : ctx.binc;

    if (ctx.movetime)
    {
        auto hard_limit = *ctx.movetime - BufferTime;
        shared.limits.time = SearchTimeManager(hard_limit, hard_limit);
    }
    else if (myTime)
    {
        auto hard_limit = *myTime - BufferTime;

        if (ctx.movestogo)
        {
            // repeating time control

            // We divide the available time by the number of movestogo (which can be zero) and then adjust
            // by 1.5x. This ensures we use more of the available time earlier.
            auto soft_limit = (*myTime - BufferTime) / (*ctx.movestogo + 1) * repeating_tc / 64;
            shared.limits.time = SearchTimeManager(soft_limit, hard_limit);
        }
        else if (myInc)
        {
            // increment time control

            // We start by using 1/30th of the remaining time plus the increment. As we move through the game we
            // use a higher proportion of the available time so that we get down to just using the increment

            auto soft_limit
                = (*myTime - BufferTime) * (blitz_tc_a + position.board().half_turn_count) / blitz_tc_b + *myInc;
            shared.limits.time = SearchTimeManager(soft_limit, hard_limit);
        }
        else
        {
            // Sudden death time control. We use 1/20th of the remaining time each turn
            auto soft_limit = (*myTime - BufferTime) * sudden_death_tc / 1024;
            shared.limits.time = SearchTimeManager(soft_limit, hard_limit);
        }
    }

    // launch search thread
    search_thread = std::thread([&]() { launch_search(position, shared); });
}

void Uci::handle_setoption_clear_hash()
{
    shared.transposition_table.clear(shared.get_threads_setting());
    shared.reset_new_game();
}

void Uci::handle_setoption_hash(int value)
{
    shared.transposition_table.set_size(value, shared.get_threads_setting());
}

void Uci::handle_setoption_threads(int value)
{
    shared.set_threads(value);
}

void Uci::handle_setoption_syzygy_path(std::string_view value)
{
    Syzygy::init(value, output_level > OutputLevel::None && finished_startup);
}

void Uci::handle_setoption_multipv(int value)
{
    shared.set_multi_pv(value);
}

void Uci::handle_setoption_chess960(bool value)
{
    shared.chess_960 = value;
}

void Uci::handle_setoption_output_level(OutputLevel level)
{
    output_level = level;
}

void Uci::handle_stop()
{
    shared.stop_searching = true;
}

void Uci::handle_quit()
{
    shared.stop_searching = true;
    quit = true;
}

void Uci::join_search_thread()
{
    if (search_thread.joinable())
        search_thread.join();
}

void Uci::process_input_stream(std::istream& is)
{
    std::string line;
    while (!quit && getline(is, line))
    {
        process_input(line);
    }
}

void Uci::process_input(std::string_view command)
{
    auto original = command;

    // We first try to handle the UCI commands that we expect to get during the search. If we cannot, then we join the
    // search thread to avoid race conditions.

    // clang-format off
    auto during_search_processor = Sequence {
    OneOf { 
        Consume { "stop", Invoke { [this] { handle_stop(); } } },
        Consume { "quit", Invoke { [this] { handle_quit(); } } } },
    EndCommand{}
    };
    // clang-format on

    if (during_search_processor(command))
    {
        return;
    }

    join_search_thread();

    // need to define this here so the lifetime extends beyond the uci_processor initialization
    auto options_handler_model = options_handler();

    using namespace std::chrono_literals;

    // clang-format off
    auto uci_processor = Sequence {
    OneOf {
        Consume { "uci", Invoke { [this]{ handle_uci(); } } },
        Consume { "ucinewgame", Invoke { [this]{ handle_ucinewgame(); } } },
        Consume { "isready", Invoke { [this]{ handle_isready(); } } },
        Consume { "position", OneOf {
            Consume { "fen", Sequence {
                TokensUntil {"moves", [this](auto fen){ return handle_position_fen(fen); } },
                Repeat { NextToken { [this](auto move){ handle_moves(move); } } } } },
            Consume { "startpos", Sequence {
                Invoke { [this] { handle_position_startpos(); } },
                OneOf {
                    Consume { "moves", Repeat { NextToken { [this](auto move){ handle_moves(move); } } } },
                    EndCommand{} } } } } },
        Consume { "go", Sequence {
            Invoke { [this]{ shared.limits = {}; } },
            WithContext { go_ctx{}, Sequence {
                Repeat { OneOf {
                    Consume { "infinite", Invoke { [](auto&){} } },
                    Consume { "wtime", NextToken { ToInt { [](auto value, auto& ctx){ ctx.wtime = value * 1ms; } } } },
                    Consume { "btime", NextToken { ToInt { [](auto value, auto& ctx){ ctx.btime = value * 1ms; } } } },
                    Consume { "winc", NextToken { ToInt { [](auto value, auto& ctx){ ctx.winc = value * 1ms; } } } },
                    Consume { "binc", NextToken { ToInt { [](auto value, auto& ctx){ ctx.binc = value * 1ms; } } } },
                    Consume { "movestogo", NextToken { ToInt { [](auto value, auto& ctx){ ctx.movestogo = value; } } } },
                    Consume { "movetime", NextToken { ToInt { [](auto value, auto& ctx){ ctx.movetime = value * 1ms; } } } },
                    Consume { "mate", NextToken { ToInt { [](auto value, auto& ctx){ ctx.mate = value; } } } },
                    Consume { "depth", NextToken { ToInt { [](auto value, auto& ctx){ ctx.depth = value; } } } },
                    Consume { "nodes", NextToken { ToInt { [](auto value, auto& ctx){ ctx.nodes = value; } } } } } },
                Invoke { [this](auto& ctx) { handle_go(ctx); } } } } } },
        Consume { "setoption", options_handler_model.build_handler() },

        // extensions
        Consume { "perft", NextToken { ToInt { [this](auto value) { PerftDivide(value, position, false); } } } },
        Consume { "test", OneOf {
            Consume { "perft", Invoke { [] { PerftSuite("test/perftsuite.txt", 0, false); } } },
            Consume { "perft960", Invoke { [] { PerftSuite("test/perft960.txt", 0, false); } } },
            Consume { "perft_legality", Invoke { [] { PerftSuite("test/perftsuite.txt", 2, true); } } },
            Consume { "perft960_legality", Invoke { [] { PerftSuite("test/perft960.txt", 3, true); } } } } },
        Consume { "bench", OneOf  {
            Sequence { EndCommand{}, Invoke { [this]{ handle_bench(10); } } },
            NextToken { ToInt { [this](auto value){ handle_bench(value); } } } } },
        Consume { "print", Invoke { [this] { handle_print(); } } },
        Consume { "spsa", Invoke { [this] { handle_spsa(); } } },
        Consume { "eval", Invoke { [this] { handle_eval(); } } },
        Consume { "probe", Invoke { [this] { handle_probe(); } } },
        Consume { "datagen", WithContext { datagen_ctx{}, Sequence {
             Repeat { OneOf {
                Consume { "output", NextToken { [](auto value, auto& ctx){ ctx.output_path = value; } } },
                Consume { "duration", NextToken { ToInt { [](auto value, auto& ctx){ ctx.duration = value * 1s;} } } } } },
            Invoke { [this](auto& ctx) { handle_datagen(ctx); } } } } } },
    EndCommand{}
    };
    // clang-format on

    if (!uci_processor(command))
    {
        std::cout << "info string unable to handle command " << std::quoted(original) << std::endl;
    }
}

void Uci::print_search_info(const SearchResults& data, bool final)
{
    if (output_level == OutputLevel::None || (output_level == OutputLevel::Minimal && !final))
    {
        return;
    }

    std::cout << "info depth " << data.depth << " seldepth " << data.sel_septh;

    if (Score(abs(data.score.value())) > Score::mate_in(MAX_DEPTH))
    {
        if (data.score > 0)
            std::cout << " score mate " << ((Score::Limits::MATE - abs(data.score.value())) + 1) / 2;
        else
            std::cout << " score mate " << -((Score::Limits::MATE - abs(data.score.value())) + 1) / 2;
    }
    else
    {
        std::cout << " score cp " << data.score.value();
    }

    if (data.type == SearchResultType::UPPER_BOUND)
        std::cout << " upperbound";
    if (data.type == SearchResultType::LOWER_BOUND)
        std::cout << " lowerbound";

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(shared.search_timer.elapsed()).count();
    auto node_count = shared.nodes();
    auto nps = node_count / std::max<int64_t>(elapsed_time, 1) * 1000;
    auto hashfull = shared.transposition_table.get_hashfull(position.board().half_turn_count);

    std::cout << " time " << elapsed_time << " nodes " << node_count << " nps " << nps << " hashfull " << hashfull
              << " tbhits " << shared.tb_hits() << " multipv " << data.multi_pv;

    std::cout << " pv "; // the current best line found

    for (const auto& move : data.pv)
    {
        if (shared.chess_960)
        {
            std::cout << format_chess960 { move } << ' ';
        }
        else
        {
            std::cout << move << ' ';
        }
    }

    std::cout << std::endl;
}

void Uci::print_bestmove(Move move)
{
    if (output_level > OutputLevel::None)
    {
        if (shared.chess_960)
        {
            std::cout << "bestmove " << format_chess960 { move } << std::endl;
        }
        else
        {
            std::cout << "bestmove " << move << std::endl;
        }
    }
}

void Uci::print_error(const std::string& error_str)
{
    std::cout << "info string Error: " << error_str << std::endl;
}

void Uci::handle_print()
{
    std::cout << position.board() << std::endl;
}

void Uci::handle_spsa()
{
    options_handler().spsa_input_print(std::cout);
}

void Uci::handle_eval()
{
    std::cout << position.board() << std::endl;
    std::cout << "Eval: " << NN::Network::slow_eval(position.board()) << std::endl;
}

void Uci::handle_probe()
{
    std::cout << position.board() << std::endl;
    auto probe = Syzygy::probe_dtz_root(position.board());

    if (!probe)
    {
        std::cout << "Failed probe" << std::endl;
        return;
    }

    std::cout << " move |   rank\n";
    std::cout << "------+---------\n";

    for (const auto& [move, tb_rank] : probe->root_moves)
    {
        std::cout << std::setw(5) << move << " | " << std::setw(7) << tb_rank << "\n";
    }

    std::cout << std::endl;
}

void Uci::handle_datagen(const datagen_ctx& ctx)
{
    datagen(ctx.output_path, ctx.duration);
}

}
