#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <time.h>
#include <vector>

#include "Benchmark.h"
#include "GameState.h"
#include "Move.h"
#include "MoveGeneration.h"
#include "MoveList.h"
#include "Network.h"
#include "Pyrrhic/tbprobe.h"
#include "Search.h"
#include "SearchData.h"
#include "TimeManage.h"
#include "TranspositionTable.h"

using namespace ::std;

void PerftSuite(std::string path, int depth_reduce, bool check_legality);
void PrintVersion();
uint64_t PerftDivide(unsigned int depth, GameState& position, bool chess960, bool check_legality);
uint64_t Perft(unsigned int depth, GameState& position, bool check_legality);
void Bench(int depth = 14);

string version = "11.4.1";

int main(int argc, char* argv[])
{
    PrintVersion();
    tb_init("<empty>");

    Network::Init();

    string Line; //to read the command given by the GUI

    GameState position;
    thread searchThread;
    ThreadSharedData data;

    for (int i = 1; i < argc; i++) //read any command line input as a regular UCI instruction
    {
        Line += argv[i];
        Line += " ";
    }

    /*Tuneable search constants*/

    double timeIncCoeffA = 40.0;
    double timeIncCoeffB = 30.0;

    //---------------------------

    while (!Line.empty() || getline(cin, Line))
    {
        istringstream iss(Line);
        string token;

        iss >> token;

        if (token == "uci")
        {
            cout << "id name Halogen " << version << endl;
            cout << "id author Kieren Pearson" << endl;
            cout << "option name Clear Hash type button" << endl;
            cout << "option name Hash type spin default 32 min 1 max 262144" << endl;
            cout << "option name Threads type spin default 1 min 1 max 256" << endl;
            cout << "option name SyzygyPath type string default <empty>" << endl;
            cout << "option name MultiPV type spin default 1 min 1 max 500" << endl;
            cout << "option name UCI_Chess960 type check default false" << endl;
            cout << "uciok" << endl;
        }

        else if (token == "isready")
            cout << "readyok" << endl;

        else if (token == "ucinewgame")
        {
            position.StartingPosition();
            tTable.ResetTable();
            data.ResetNewGame();
        }

        else if (token == "position")
        {
            position.Reset();
            position.StartingPosition();

            iss >> token;

            if (token == "fen")
            {
                vector<string> fen;

                while (iss >> token && token != "moves")
                {
                    fen.push_back(token);
                }

                if (!position.InitialiseFromFen(fen))
                    cout << "BAD FEN" << endl;
                if (token == "moves")
                    while (iss >> token)
                        position.ApplyMove(token);
            }

            if (token == "startpos")
            {
                iss >> token;
                if (token == "moves")
                    while (iss >> token)
                        position.ApplyMove(token);
            }
        }

        else if (token == "go")
        {
            SearchLimits limits;

            int wtime = 0;
            int btime = 0;
            int winc = 0;
            int binc = 0;
            int movestogo = 0;

            while (iss >> token)
            {
                if (token == "wtime")
                    iss >> wtime;
                else if (token == "btime")
                    iss >> btime;
                else if (token == "winc")
                    iss >> winc;
                else if (token == "binc")
                    iss >> binc;
                else if (token == "movestogo")
                    iss >> movestogo;

                else if (token == "mate")
                {
                    int mate = 0;
                    iss >> mate;
                    limits.SetMateLimit(mate);
                }

                else if (token == "depth")
                {
                    int depth = 0;
                    iss >> depth;
                    limits.SetDepthLimit(depth);
                }

                else if (token == "infinite")
                {
                    limits.SetInfinite();
                }

                else if (token == "movetime")
                {
                    int searchTime = 0;
                    iss >> searchTime;
                    limits.SetTimeLimits(searchTime, searchTime);
                }

                else if (token == "nodes")
                {
                    uint64_t nodes = 0;
                    iss >> nodes;
                    limits.SetNodeLimit(nodes);
                }
            }

            int myTime = position.Board().stm ? wtime : btime;
            int myInc = position.Board().stm ? winc : binc;

            if (myTime != 0)
            {
                int AllocatedTime = 0;

                if (movestogo != 0)
                    AllocatedTime = myTime / (movestogo + 1) * 3 / 2; //repeating time control
                else if (myInc != 0)
                    // use a greater proportion of remaining time as the game continues, so that we use it all up and get to just increment
                    AllocatedTime = myTime * (1 + position.Board().half_turn_count / timeIncCoeffA) / timeIncCoeffB + myInc; //increment time control
                else
                    AllocatedTime = myTime / 20; //sudden death time control

                limits.SetTimeLimits(myTime, AllocatedTime);
            }

            if (searchThread.joinable())
                searchThread.join();

            data.SetLimits(std::move(limits));
            searchThread = thread([position, &data] { SearchThread(position, data); });
        }

        else if (token == "setoption")
        {
            iss >> token; //'name'
            iss >> token;

            if (token == "Clear")
            {
                iss >> token;
                if (token == "Hash")
                {
                    tTable.ResetTable();
                    data.ResetNewGame();
                }
            }

            else if (token == "Hash")
            {
                iss >> token; //'value'
                iss >> token;
                tTable.SetSize(stoi(token));
            }

            else if (token == "Threads")
            {
                iss >> token; //'value'
                iss >> token;
                data.SetThreads(stoi(token));
            }

            else if (token == "SyzygyPath")
            {
                iss >> token; //'value'
                iss >> token;

                tb_init(token.c_str());
            }

            else if (token == "MultiPV")
            {
                iss >> token; //'value'
                iss >> token;
                data.SetMultiPv(stoi(token));
            }

            else if (token == "UCI_Chess960")
            {
                iss >> token; //'value'
                iss >> token;
                std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c) { return std::tolower(c); });
                data.SetChess960(token == "true" ? true : false);
            }

            else if (token == "timeIncCoeffA")
            {
                iss >> token; //'value'
                iss >> token;
                timeIncCoeffA = stod(token);
            }

            else if (token == "timeIncCoeffB")
            {
                iss >> token; //'value'
                iss >> token;
                timeIncCoeffB = stod(token);
            }
        }

        else if (token == "perft")
        {
            iss >> token;
            PerftDivide(stoi(token), position, data.GetParameters().chess960, false);
        }

        else if (token == "test")
        {
            iss >> token;

            if (token == "perft")
                PerftSuite("perftsuite.txt", 0, false);

            if (token == "perft960")
                PerftSuite("perft960.txt", 0, false);

            if (token == "perft_legality")
                PerftSuite("perftsuite.txt", 2, true);

            if (token == "perft960_legality")
                PerftSuite("perft960.txt", 3, true);
        }

        else if (token == "stop")
        {
            KeepSearching = false;
            if (searchThread.joinable())
                searchThread.join();
        }

        else if (token == "quit")
        {
            KeepSearching = false;
            if (searchThread.joinable())
                searchThread.join();
            return 0;
        }

        else if (token == "bench")
        {
            if (iss >> token)
                Bench(stoi(token));
            else
                Bench();
        }

        //Non uci commands
        else if (token == "print")
            position.Board().Print();
        else
            cout << "Unknown command" << endl;

        Line.clear();

        if (argc != 1) //Temporary fix to quit after a command line UCI argument is done
            break;
    }

    if (searchThread.joinable())
        searchThread.join();

    return 0;
}

void PrintVersion()
{
    cout << "Halogen " << version;

#if defined(_WIN64) or defined(__x86_64__)
    cout << " x64";

#if defined(USE_POPCNT) && !defined(USE_PEXT)
    cout << " POPCNT";
#endif

#if defined(USE_PEXT)
    cout << " PEXT";
#endif

#if defined(USE_AVX2)
    cout << " AVX2";
#endif

    cout << endl;

#elif defined(_WIN32)
    cout << " x86" << endl;
#else
    cout << " UNKNOWN COMPILATION" << endl;
#endif
}

void PerftSuite(std::string path, int depth_reduce, bool check_legality)
{
    ifstream infile(path);

    unsigned int Perfts = 0;
    unsigned int Correct = 0;
    double Totalnodes = 0;
    GameState position;
    string line;

    auto before = std::chrono::steady_clock::now();
    while (getline(infile, line))
    {
        vector<string> arrayTokens;
        istringstream iss(line);
        arrayTokens.clear();

        do
        {
            string stub;
            iss >> stub;
            arrayTokens.push_back(stub);
        } while (iss);

        string fen = arrayTokens[0] + " "
            + arrayTokens[1] + " "
            + arrayTokens[2] + " "
            + arrayTokens[3] + " "
            + arrayTokens[4] + " "
            + arrayTokens[5];

        position.InitialiseFromFen(fen);

        int depth = (arrayTokens.size() - 7) / 2 - depth_reduce;
        uint64_t nodes = Perft(depth, position, check_legality);
        uint64_t correct = stoull(arrayTokens.at(arrayTokens.size() - 2 * (1 + depth_reduce)));
        if (nodes == stoull(arrayTokens.at(arrayTokens.size() - 2 * (1 + depth_reduce))))
        {
            cout << "CORRECT   (" << nodes << " == " << correct << ") [" << fen << "] depth: " << depth << std::endl;
            Correct++;
        }
        else
        {
            cout << "INCORRECT (" << nodes << " != " << correct << ") [" << fen << "] depth: " << depth << std::endl;
        }

        Totalnodes += nodes;
        Perfts++;
    }
    auto after = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(after - before).count();

    cout << "\n\nCompleted perft with: " << Correct << "/" << Perfts << " correct";
    cout << "\nTotal nodes: " << (Totalnodes) << " in " << duration << "s";
    cout << "\nNodes per second: " << static_cast<unsigned int>(Totalnodes / duration);
    std::cout << std::endl;
}

uint64_t PerftDivide(unsigned int depth, GameState& position, bool chess960, bool check_legality)
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

        if (chess960)
        {
            moves[i].Print960(position.Board().stm, position.Board().castle_squares);
        }
        else
        {
            moves[i].Print();
        }

        cout << ": " << ChildNodeCount << endl;
        nodeCount += ChildNodeCount;
    }

    auto after = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(after - before).count();

    cout << "\nNodes searched: " << (nodeCount) << " in " << duration << " seconds ";
    cout << "(" << static_cast<unsigned int>(nodeCount / duration) << " nps)" << endl;
    return nodeCount;
}

uint64_t Perft(unsigned int depth, GameState& position, bool check_legality)
{
    if (depth == 0)
        return 1; //if perftdivide is called with 1 this is necesary

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
                position.Board().Print();
                move.Print();
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

void Bench(int depth)
{
    Timer timer;

    uint64_t nodeCount = 0;
    GameState position;
    SearchLimits limits;
    limits.SetDepthLimit(depth);
    ThreadSharedData data(std::move(limits));

    for (size_t i = 0; i < benchMarkPositions.size(); i++)
    {
        if (!position.InitialiseFromFen(benchMarkPositions[i]))
        {
            cout << "BAD FEN!" << endl;
            break;
        }

        tTable.ResetTable();
        data.ResetNewGame();
        nodeCount += SearchThread(position, data);
    }

    cout << nodeCount << " nodes " << int(nodeCount / max(timer.ElapsedMs(), 1) * 1000) << " nps" << endl;
}
