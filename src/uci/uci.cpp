#include "uci.h"

#include "../Benchmark.h"
#include "../EGTB.h"
#include "../GameState.h"
#include "../MoveGeneration.h"
#include "../SearchData.h"
#include "parse.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

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
                std::cout << position.Board() << move;
                std::cout << std::endl;
                std::cout << present << " " << legal << std::endl;
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

void Bench(int depth)
{
    Timer timer;

    uint64_t nodeCount = 0;
    GameState position;
    SearchSharedState data(1);
    data.limits.depth = depth;

    for (size_t i = 0; i < benchMarkPositions.size(); i++)
    {
        if (!position.InitialiseFromFen(benchMarkPositions[i]))
        {
            std::cout << "BAD FEN!" << std::endl;
            break;
        }

        data.limits.time.reset();
        SearchThread(position, data);
        nodeCount += data.nodes();
    }

    int elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(timer.elapsed()).count();
    std::cout << nodeCount << " nodes " << nodeCount / std::max(elapsed_time, 1) * 1000 << " nps" << std::endl;
}

Uci::Uci(std::string_view version)
    : version_(version)
{
}

Uci::~Uci()
{
    join_search_thread();
}

void Uci::handle_uci()
{
    std::cout << "id name Halogen " << version_ << "\n";
    std::cout << "id author Kieren Pearson\n";
    std::cout << "option name Clear Hash type button\n";
    std::cout << "option name Hash type spin default 32 min 1 max 262144\n";
    std::cout << "option name Threads type spin default 1 min 1 max 256\n";
    std::cout << "option name SyzygyPath type string default <empty>\n";
    std::cout << "option name MultiPV type spin default 1 min 1 max 500\n";
    std::cout << "option name UCI_Chess960 type check default false\n";
    std::cout << "uciok\n";
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

void Uci::handle_go(go_ctx& ctx)
{
    using namespace std::chrono_literals;

    // The amount of time we leave on the clock for safety
    constexpr static auto BufferTime = 100ms;

    // Tuneable time constants

    constexpr static int timeIncCoeffA = 40;
    constexpr static int timeIncCoeffB = 1200;

    auto myTime = (position.Board().stm ? ctx.wtime : ctx.btime) * 1ms;
    auto myInc = (position.Board().stm ? ctx.winc : ctx.binc) * 1ms;

    if (ctx.movetime != 0)
    {
        auto hard_limit = (ctx.movetime) * 1ms - BufferTime;
        shared.limits.time = SearchTimeManager(hard_limit, hard_limit);
    }
    else if (myTime != 0ms)
    {
        auto hard_limit = myTime - BufferTime;

        if (ctx.movestogo != 0)
        {
            // repeating time control

            // We divide the available time by the number of movestogo (which can be zero) and then adjust
            // by 1.5x. This ensures we use more of the available time earlier.
            auto soft_limit = (myTime - BufferTime) / (ctx.movestogo + 1) * 3 / 2;
            shared.limits.time = SearchTimeManager(soft_limit, hard_limit);
        }
        else if (myInc != 0ms)
        {
            // increment time control

            // We start by using 1/30th of the remaining time plus the increment. As we move through the game we
            // use a higher proportion of the available time so that we get down to just using the increment

            auto soft_limit
                = (myTime - BufferTime) * (timeIncCoeffA + position.Board().half_turn_count) / timeIncCoeffB + myInc;
            shared.limits.time = SearchTimeManager(soft_limit, hard_limit);
        }
        else
        {
            // Sudden death time control. We use 1/20th of the remaining time each turn
            auto soft_limit = (myTime - BufferTime) / 20;
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

bool Uci::handle_setoption_hash(int value)
{
    if (GetBitCount(value) != 1)
    {
        std::cout << "info error transposition table size must be a power of two" << std::endl;
        return false;
    }

    tTable.SetSize(value);
    return true;
}

void Uci::handle_setoption_threads(int value)
{
    shared.set_threads(value);
}

void Uci::handle_setoption_syzygy_path(std::string_view value)
{
    Syzygy::init(value);
}

void Uci::handle_setoption_multipv(int value)
{
    shared.set_multi_pv(value);
}

void Uci::handle_setoption_chess960(std::string_view token)
{
    shared.chess_960 = (token == "true");
}

void Uci::handle_stop()
{
    KeepSearching = false;
}

void Uci::handle_quit()
{
    KeepSearching = false;
    quit = true;
}

void Uci::join_search_thread()
{
    if (searchThread.joinable())
        searchThread.join();
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

    if (during_search_processor.handle(command))
    {
        return;
    }

    join_search_thread();

    // clang-format off
    auto uci_processor = sequence { 
    one_of { 
        consume { "ucinewgame", invoke { [this]{ handle_ucinewgame(); } } },
        consume { "uci", invoke { [this]{ handle_uci(); } } },
        consume { "isready", invoke { [this]{ handle_isready(); } } },
        consume { "position", one_of { 
            consume { "fen", sequence {
                tokens_until {"moves", [this](auto fen){ return position.InitialiseFromFen(fen); } },
                repeat { next_token { [this](auto move){ position.ApplyMove(move); } } } } },
            consume { "startpos", sequence { 
                invoke { [this] { position.StartingPosition(); } }, 
                one_of {
                    consume { "moves", repeat { next_token { [this](auto move){ position.ApplyMove(move); } } } },
                    end_command{} } } } } },
        consume { "go", sequence {
            invoke { [this]{ shared.limits = {}; } },
            with_context { go_ctx{}, sequence {
                repeat { one_of {
                    consume { "infinite", invoke { [](auto&){} } },
                    consume { "wtime", next_token { to_int { [](auto value, auto& ctx){ ctx.wtime = value; } } } },
                    consume { "btime", next_token { to_int { [](auto value, auto& ctx){ ctx.btime = value; } } } },
                    consume { "winc", next_token { to_int { [](auto value, auto& ctx){ ctx.winc = value; } } } },
                    consume { "binc", next_token { to_int { [](auto value, auto& ctx){ ctx.binc = value; } } } },
                    consume { "movestogo", next_token { to_int { [](auto value, auto& ctx){ ctx.movestogo = value; } } } },
                    consume { "movetime", next_token { to_int { [](auto value, auto& ctx){ ctx.movetime = value; } } } },
                    consume { "mate", next_token { to_int { [&](auto value, auto&){ shared.limits.mate = value; } } } },
                    consume { "depth", next_token { to_int { [&](auto value, auto&){ shared.limits.depth = value; } } } },
                    consume { "nodes", next_token { to_int { [&](auto value, auto&){ shared.limits.nodes = value; } } } } } },
                invoke { [this](auto& ctx) { handle_go(ctx); } } } } } },
        consume { "setoption name Clear Hash", invoke { [this]{ handle_setoption_clear_hash(); } } },
        consume { "setoption name Hash value", next_token { to_int { [this](auto value) { return handle_setoption_hash(value); } } } },
        consume { "setoption name Threads value", next_token { to_int { [this](auto value) { handle_setoption_threads(value); }  } } },
        consume { "setoption name SyzygyPath value", next_token { [this](auto str) { handle_setoption_syzygy_path(str); } } },
        consume { "setoption name MultiPV value", next_token { to_int { [this](auto value) { handle_setoption_multipv(value); } } } },
        consume { "setoption name UCI_Chess960 value", next_token { [this](auto str) { handle_setoption_chess960(str); } } },

        // extensions
        consume { "perft", next_token { to_int { [this](auto value) { Perft(value, position, false); } } } },
        consume { "test", one_of { 
            consume { "perft", invoke { [] { PerftSuite("test/perftsuite.txt", 0, false); } } },
            consume { "perft960", invoke { [] { PerftSuite("test/perft960.txt", 0, false); } } },
            consume { "perft_legality", invoke { [] { PerftSuite("test/perftsuite.txt", 2, true); } } },
            consume { "perft960_legality", invoke { [] { PerftSuite("test/perft960.txt", 3, true); } } } } },
        consume { "bench", one_of  { 
            sequence { end_command{}, invoke { []{ Bench(10); } } },
            next_token { to_int { [](auto value){ Bench(value); } } } } },
        consume { "print", invoke { [this](){ std::cout << position.Board() << std::endl; } } } },
    end_command{}
    };
    // clang-format on

    if (!uci_processor.handle(command))
    {
        std::cout << "info string unable to handle command " << std::quoted(original) << std::endl;
    }
}