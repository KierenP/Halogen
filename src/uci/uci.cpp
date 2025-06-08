#include "uci.h"

#include "../Benchmark.h"
#include "../EGTB.h"
#include "../GameState.h"
#include "../MoveGeneration.h"
#include "../SearchConstants.h"
#include "../SearchData.h"
#include "../datagen/convert_to_viribinpack.h"
#include "../datagen/datagen.h"
#include "../datagen/filter.h"
#include "../datagen/syzygy_rescore.h"
#include "options.h"
#include "parse.h"


#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>

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

uint64_t Perft(unsigned int depth, GameState& position, bool check_legality)
{
    if (depth == 0)
        return 1; // if perftdivide is called with 1 this is necesary

    uint64_t nodeCount = 0;
    BasicMoveList moves;
    LegalMoves(position.Board(), moves);

    if (check_legality)
    {
        for (int i = 0; i < UINT16_MAX; i++)
        {
            Move move(i);
            bool legal = MoveIsLegal(position.Board(), move);

            bool present = std::find(moves.begin(), moves.end(), move) != moves.end();

            if (present != legal)
            {
                std::cout << position.Board() << move << "\n";
                std::cout << present << " " << legal << "\n";
                std::cout << move.GetFrom() << " " << move.GetTo() << " " << move.GetFlag() << std::endl;
                return 0; // cause perft answer to be incorrect
            }
        }
    }

    if (depth == 1)
        return moves.size();

    for (size_t i = 0; i < moves.size(); i++)
    {
        position.ApplyMove(moves[i]);
        nodeCount += Perft(depth - 1, position, check_legality);
        position.RevertMove();
    }

    return nodeCount;
}

void PerftSuite(std::string path, int depth_reduce, bool check_legality)
{
    std::ifstream infile(path);

    unsigned int Perfts = 0;
    unsigned int Correct = 0;
    double Totalnodes = 0;
    GameState position;
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

        position.InitialiseFromFen(fen);

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
    std::cout << "\nNodes per second: " << static_cast<unsigned int>(Totalnodes / duration);
    std::cout << std::endl;
}

uint64_t PerftDivide(unsigned int depth, GameState& position, bool check_legality)
{
    auto before = std::chrono::steady_clock::now();

    uint64_t nodeCount = 0;
    BasicMoveList moves;
    LegalMoves(position.Board(), moves);

    for (size_t i = 0; i < moves.size(); i++)
    {
        position.ApplyMove(moves[i]);
        uint64_t ChildNodeCount = Perft(depth - 1, position, check_legality);
        position.RevertMove();
        std::cout << moves[i] << ": " << ChildNodeCount << std::endl;
        nodeCount += ChildNodeCount;
    }

    auto after = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(after - before).count();

    std::cout << "\nNodes searched: " << (nodeCount) << " in " << duration << " seconds ";
    std::cout << "(" << static_cast<unsigned int>(nodeCount / duration) << " nps)" << std::endl;
    return nodeCount;
}

void Uci::handle_bench(int depth)
{
    Timer timer;

    uint64_t nodeCount = 0;
    shared.limits.depth = depth;

    for (size_t i = 0; i < benchMarkPositions.size(); i++)
    {
        if (!position.InitialiseFromFen(benchMarkPositions[i]))
        {
            std::cout << "BAD FEN!" << std::endl;
            break;
        }

        shared.limits.time.reset();
        SearchThread(position, shared);
        nodeCount += shared.nodes();
    }

    int elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(timer.elapsed()).count();
    std::cout << nodeCount << " nodes " << nodeCount / std::max(elapsed_time, 1) * 1000 << " nps" << std::endl;
}

auto Uci::options_handler()
{
#define tuneable_int(name, default_, min_, max_)                                                                       \
    spin_option                                                                                                        \
    {                                                                                                                  \
        #name, default_, min_, max_, [](auto value) { name = value; }                                                  \
    }

#define tuneable_float(name, default_, min_, max_)                                                                     \
    float_option                                                                                                       \
    {                                                                                                                  \
        #name, default_, min_, max_, [](auto value) { name = value; }                                                  \
    }

    return uci_options {
        button_option { "Clear Hash", [this] { handle_setoption_clear_hash(); } },
        check_option { "UCI_Chess960", false, [this](bool value) { handle_setoption_chess960(value); } },
        spin_option { "Hash", 32, 1, 262144, [this](auto value) { handle_setoption_hash(value); } },
        spin_option { "Threads", 1, 1, 256, [this](auto value) { handle_setoption_threads(value); } },
        spin_option { "MultiPV", 1, 1, 256, [this](auto value) { handle_setoption_multipv(value); } },
        string_option { "SyzygyPath", "<empty>", [this](auto value) { handle_setoption_syzygy_path(value); } },
        combo_option {
            "OutputLevel", OutputLevel::Default, [this](auto value) { handle_setoption_output_level(value); } },
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
    position.StartingPosition();
    tTable.ResetTable();
    shared.ResetNewGame();
}

bool Uci::handle_position_fen(std::string_view fen)
{
    return position.InitialiseFromFen(fen);
}

void Uci::handle_moves(std::string_view move)
{
    position.ApplyMove(move);
}

void Uci::handle_position_startpos()
{
    position.StartingPosition();
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

    // Tuneable time constants
    constexpr static int timeIncCoeffA = 40;
    constexpr static int timeIncCoeffB = 1200;

    const auto& myTime = position.Board().stm ? ctx.wtime : ctx.btime;
    const auto& myInc = position.Board().stm ? ctx.winc : ctx.binc;

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
            auto soft_limit = (*myTime - BufferTime) / (*ctx.movestogo + 1) * 3 / 2;
            shared.limits.time = SearchTimeManager(soft_limit, hard_limit);
        }
        else if (myInc)
        {
            // increment time control

            // We start by using 1/30th of the remaining time plus the increment. As we move through the game we
            // use a higher proportion of the available time so that we get down to just using the increment

            auto soft_limit
                = (*myTime - BufferTime) * (timeIncCoeffA + position.Board().half_turn_count) / timeIncCoeffB + *myInc;
            shared.limits.time = SearchTimeManager(soft_limit, hard_limit);
        }
        else
        {
            // Sudden death time control. We use 1/20th of the remaining time each turn
            auto soft_limit = (*myTime - BufferTime) / 20;
            shared.limits.time = SearchTimeManager(soft_limit, hard_limit);
        }
    }

    // launch search thread
    searchThread = std::thread([&]() { SearchThread(position, shared); });
}

void Uci::handle_setoption_clear_hash()
{
    tTable.ResetTable();
    shared.ResetNewGame();
}

void Uci::handle_setoption_hash(int value)
{
    tTable.SetSize(value);
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
    if (searchThread.joinable())
        searchThread.join();
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
    auto during_search_processor = sequence {
    one_of { 
        consume { "stop", invoke { [this] { handle_stop(); } } },
        consume { "quit", invoke { [this] { handle_quit(); } } } },
    end_command{}
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
    auto uci_processor = sequence {
    one_of {
        consume { "uci", invoke { [this]{ handle_uci(); } } },
        consume { "ucinewgame", invoke { [this]{ handle_ucinewgame(); } } },
        consume { "isready", invoke { [this]{ handle_isready(); } } },
        consume { "position", one_of {
            consume { "fen", sequence {
                tokens_until {"moves", [this](auto fen){ return handle_position_fen(fen); } },
                repeat { next_token { [this](auto move){ handle_moves(move); } } } } },
            consume { "startpos", sequence {
                invoke { [this] { handle_position_startpos(); } },
                one_of {
                    consume { "moves", repeat { next_token { [this](auto move){ handle_moves(move); } } } },
                    end_command{} } } } } },
        consume { "go", sequence {
            invoke { [this]{ shared.limits = {}; } },
            with_context { go_ctx{}, sequence {
                repeat { one_of {
                    consume { "infinite", invoke { [](auto&){} } },
                    consume { "wtime", next_token { to_int { [](auto value, auto& ctx){ ctx.wtime = value * 1ms; } } } },
                    consume { "btime", next_token { to_int { [](auto value, auto& ctx){ ctx.btime = value * 1ms; } } } },
                    consume { "winc", next_token { to_int { [](auto value, auto& ctx){ ctx.winc = value * 1ms; } } } },
                    consume { "binc", next_token { to_int { [](auto value, auto& ctx){ ctx.binc = value * 1ms; } } } },
                    consume { "movestogo", next_token { to_int { [](auto value, auto& ctx){ ctx.movestogo = value; } } } },
                    consume { "movetime", next_token { to_int { [](auto value, auto& ctx){ ctx.movetime = value * 1ms; } } } },
                    consume { "mate", next_token { to_int { [](auto value, auto& ctx){ ctx.mate = value; } } } },
                    consume { "depth", next_token { to_int { [](auto value, auto& ctx){ ctx.depth = value; } } } },
                    consume { "nodes", next_token { to_int { [](auto value, auto& ctx){ ctx.nodes = value; } } } } } },
                invoke { [this](auto& ctx) { handle_go(ctx); } } } } } },
        consume { "setoption", options_handler_model.build_handler() },

        // extensions
        consume { "perft", next_token { to_int { [this](auto value) { PerftDivide(value, position, false); } } } },
        consume { "test", one_of {
            consume { "perft", invoke { [] { PerftSuite("test/perftsuite.txt", 0, false); } } },
            consume { "perft960", invoke { [] { PerftSuite("test/perft960.txt", 0, false); } } },
            consume { "perft_legality", invoke { [] { PerftSuite("test/perftsuite.txt", 2, true); } } },
            consume { "perft960_legality", invoke { [] { PerftSuite("test/perft960.txt", 3, true); } } } } },
        consume { "bench", one_of  {
            sequence { end_command{}, invoke { [this]{ handle_bench(10); } } },
            next_token { to_int { [this](auto value){ handle_bench(value); } } } } },
        consume { "print", invoke { [this] { handle_print(); } } },
        consume { "spsa", invoke { [this] { handle_spsa(); } } },
        consume { "eval", invoke { [this] { handle_eval(); } } },
        consume { "probe", invoke { [this] { handle_probe(); } } },
        consume { "datagen", with_context { datagen_ctx{}, sequence {
             repeat { one_of {
                consume { "output", next_token { [](auto value, auto& ctx){ ctx.output_path = value; } } },
                consume { "duration", next_token { to_int{[](auto value, auto& ctx){ ctx.duration = value * 1s;} } } } } },
            invoke { [this](auto& ctx) { handle_datagen(ctx); } } } } },
        consume { "syzygy_rescore", with_context { file_io_ctx{}, sequence {
             repeat { one_of {
                consume { "input", next_token { [](auto value, auto& ctx){ ctx.input_path = value; } } },
                consume { "output", next_token { [](auto value, auto& ctx){ ctx.output_path = value; } } } } },
            invoke { [this](auto& ctx) { handle_syzygy_rescore(ctx); } } } } },
        consume { "filter", with_context { file_io_ctx{}, sequence {
             repeat { one_of {
                consume { "input", next_token { [](auto value, auto& ctx){ ctx.input_path = value; } } },
                consume { "output", next_token { [](auto value, auto& ctx){ ctx.output_path = value; } } } } },
            invoke { [this](auto& ctx) { handle_filter(ctx); } } } } },
        consume { "viribinpack_convert", with_context { file_io_ctx{}, sequence {
            repeat { one_of {
                consume { "input", next_token { [](auto value, auto& ctx){ ctx.input_path = value; } } },
                consume { "output", next_token { [](auto value, auto& ctx){ ctx.output_path = value; } } } } },
            invoke { [this](auto& ctx) { handle_viribinpack_convert(ctx); } } } } } },
    end_command{}
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
    auto hashfull = tTable.GetCapacity(position.Board().half_turn_count);

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

void Uci::handle_print()
{
    std::cout << position.Board() << std::endl;
}

void Uci::handle_spsa()
{
    options_handler().spsa_input_print(std::cout);
}

void Uci::handle_eval()
{
    std::cout << position.Board() << std::endl;
    std::cout << "Eval: " << Network::SlowEval(position.Board()) << std::endl;
}

void Uci::handle_probe()
{
    std::cout << position.Board() << std::endl;
    auto probe = Syzygy::probe_dtz_root(position.Board());

    if (!probe)
    {
        std::cout << "Failed probe" << std::endl;
        return;
    }

    std::cout << " move |   score   |   rank\n";
    std::cout << "------+-----------+---------\n";

    for (const auto& [move, tb_score, tb_rank] : probe->root_moves)
    {
        std::cout << std::setw(5) << move << " | " << std::setw(6) << tb_score << "  | " << std::setw(7) << tb_rank
                  << "\n";
    }

    std::cout << std::endl;
}

void Uci::handle_datagen(const datagen_ctx& ctx)
{
    datagen(*this, ctx.output_path, ctx.duration);
}

void Uci::handle_syzygy_rescore(const file_io_ctx& ctx)
{
    syzygy_rescore(ctx.input_path, ctx.output_path);
}

void Uci::handle_filter(const file_io_ctx& ctx)
{
    filter(ctx.input_path, ctx.output_path);
}

void Uci::handle_viribinpack_convert(const file_io_ctx& ctx)
{
    convert_to_viribinpack(ctx.input_path, ctx.output_path);
}
