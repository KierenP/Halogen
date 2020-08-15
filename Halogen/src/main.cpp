#include "Benchmark.h"
#include "Texel.h"
#include <thread>

using namespace::std;

void PerftSuite();
void PrintVersion();
uint64_t PerftDivide(unsigned int depth, Position& position);
uint64_t Perft(unsigned int depth, Position& position);
void Bench();

string version = "6"; 

int main(int argc, char* argv[])
{
	PrintVersion();

	unsigned long long init[4] = { 0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL }, length = 4;
	init_by_array64(init, length);

	ZobristInit();
	BBInit();
	EvalInit();

	string Line;					//to read the command given by the GUI
	cout.setf(ios::unitbuf);		// Make sure that the outputs are sent straight away to the GUI

	EvaluateDebug();				//uncomment for debug purposes. Must be run in debug mode to work
	//PerftSuite();

	tTable.SetSize(1);
	pawnHashTable.Init(1);

	Position position;
	position.StartingPosition();

	unsigned int ThreadCount = 1;

	if (argc == 2 && strcmp(argv[1], "bench") == 0) { Bench(); return 0; }	//currently only supports bench from command line for openBench integration

	while (getline(cin, Line))
	{
		istringstream iss(Line);
		string token;

		iss >> token;

		if (token == "uci")
		{
			cout << "id name Halogen " << version << endl;
			cout << "id author Kieren Pearson" << endl;
			cout << "option name Clear Hash type button" << endl;
			cout << "option name Hash type spin default 2 min 2 max 8192" << endl;
			cout << "option name Threads type spin default 1 min 1 max 8" << endl;
			cout << "uciok" << endl;
		}

		else if (token == "isready") cout << "readyok" << endl;

		else if (token == "ucinewgame")
		{
			position.StartingPosition();
			pawnHashTable.ResetTable();
			tTable.ResetTable();
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

				if (!position.InitialiseFromFen(fen)) cout << "BAD FEN" << endl;
				if (token == "moves") while (iss >> token) position.ApplyMove(token);
			}

			if (token == "startpos")
			{
				iss >> token;
				if (token == "moves") while (iss >> token) position.ApplyMove(token);
			}
		}

		else if (token == "go")
		{
			int wtime = 0;
			int btime = 0;
			int winc = 0;
			int binc = 0;
			int searchTime = 0;
			int movestogo = 0;

			while (iss >> token)
			{
				if (token == "wtime")	iss >> wtime;
				else if (token == "btime")	iss >> btime;
				else if (token == "winc")	iss >> winc;
				else if (token == "binc")	iss >> binc;
				else if (token == "movetime") iss >> searchTime;
				else if (token == "infinite") searchTime = 2147483647;
				else if (token == "movestogo") iss >> movestogo;
			}

			int movetime = 0;

			if (searchTime != 0) 
				movetime = searchTime;
			else
			{
				if (movestogo == 0)
				{

					if (position.GetTurn() == WHITE)
						movetime = wtime / 20 + winc;
					else
						movetime = btime / 20 + binc;
				}
				else
				{
					if (position.GetTurn() == WHITE)
						movetime = movestogo <= 1 ? wtime : wtime / (movestogo + 1) * 2;	
					else
						movetime = movestogo <= 1 ? btime : btime / (movestogo + 1) * 2;
				}
			}

			thread searchThread([&] {MultithreadedSearch(position, movetime, ThreadCount); });
			searchThread.detach();
			
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
					pawnHashTable.ResetTable();
					tTable.ResetTable();
				}
			}

			else if (token == "Hash")
			{
				iss >> token; //'value'
				iss >> token;

				int size = stoi(token);

				if (size < 2)
					cout << "info string Hash size too small" << endl;
				else if (size > 8192)
					cout << "info string Hash size too large" << endl;
				else
				{
					tTable.SetSize(stoi(token) - 1);
					pawnHashTable.Init(1);
				}
			}

			else if (token == "Threads")
			{
				iss >> token; //'value'
				iss >> token;

				int size = stoi(token);

				if (size < 1)
					cout << "info string thread count too small" << endl;
				else if (size > 8)
					cout << "info string thread count too large" << endl;
				else
				{
					ThreadCount = size;
				}
			}
		}

		else if (token == "perft")
		{
			iss >> token;
			PerftDivide(stoi(token), position);
		}

		else if (token == "texel")
		{
			Texel(TexelParamiters(), TexelPST());
		}

		else if (token == "stop") KeepSearching = false;
		else if (token == "print") position.Print();
		else if (token == "quit") return 0;
		else if (token == "bench") Bench();
		
		else cout << "Unknown command" << endl;
	}

	return 0;
}

void PrintVersion()
{
#if defined(_WIN64)
	#if defined(_MSC_VER) || defined(USE_POPCNT)
		cout << "Halogen " << version << " x64 POPCNT" << endl;
	#else
		cout << "Halogen " << version << " x64" << endl;
	#endif 
#elif defined(_WIN32)
	cout << "Halogen " << version << " x86" << endl;
#else
	cout << "Halogen " << version << " UNKNOWN COMPILATION" << endl;
#endif
}

void PerftSuite()
{
	ifstream infile("perftsuite.txt");

	//multi-coloured text in concole 
	HANDLE  hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	unsigned int Perfts = 0;
	unsigned int Correct = 0;
	double Totalnodes = 0;
	Position position;
	string line;

	clock_t before = clock();
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

		position.InitialiseFromFen(line);
		
		uint64_t nodes = Perft((arrayTokens.size() - 7) / 2, position);
		if (nodes == stoull(arrayTokens.at(arrayTokens.size() - 2)))
		{
			SetConsoleTextAttribute(hConsole, 2);	//green text
			cout << "\nCORRECT Perft with depth " << (arrayTokens.size() - 7) / 2 << " = " << nodes << " leaf nodes";
			SetConsoleTextAttribute(hConsole, 7);	//back to gray
			Correct++;
		}
		else
		{
			SetConsoleTextAttribute(hConsole, 4);	//red text
			cout << "\nINCORRECT Perft with depth " << (arrayTokens.size() - 7) / 2 << " = " << nodes << " leaf nodes";
			SetConsoleTextAttribute(hConsole, 7);	//back to gray
		}

		Totalnodes += nodes;
		Perfts++;
	}
	clock_t after = clock();

	double elapsed_ms = (double(after) - double(before)) / CLOCKS_PER_SEC * 1000;

	cout << "\n\nCompleted perft with: " << Correct << "/" << Perfts << " correct";
	cout << "\nTotal nodes: " << (Totalnodes) << " in " << (elapsed_ms / 1000) << "s";
	cout << "\nNodes per second: " << static_cast<unsigned int>((Totalnodes / elapsed_ms) * 1000);
}

uint64_t PerftDivide(unsigned int depth, Position& position)
{
	clock_t before = clock();

	uint64_t nodeCount = 0;
	vector<Move> moves;
	LegalMoves(position, moves);

	for (size_t i = 0; i < moves.size(); i++)
	{
		position.ApplyMove(moves.at(i));
		uint64_t ChildNodeCount = Perft(depth - 1, position);
		position.RevertMove();

		moves.at(i).Print();
		cout << ": " << ChildNodeCount << endl;
		nodeCount += ChildNodeCount;
	}

	clock_t after = clock();
	double elapsed_ms = (double(after) - double(before)) / CLOCKS_PER_SEC * 1000;

	cout << "\nTotal nodes: " << (nodeCount) << " in " << (elapsed_ms / 1000) << "s";
	cout << "\nNodes per second: " << static_cast<unsigned int>((nodeCount / elapsed_ms) * 1000);
	return nodeCount;
}

uint64_t Perft(unsigned int depth, Position& position)
{
	if (depth == 0)
		return 1;	//if perftdivide is called with 1 this is necesary

	uint64_t nodeCount = 0;
	vector<Move> moves;
	LegalMoves(position, moves);

	for (size_t i = 0; i < moves.size(); i++)
	{
		position.ApplyMove(moves.at(i));
		nodeCount += Perft(depth - 1, position);
		position.RevertMove();
	}

	return nodeCount;
}

void Bench()
{
	Timer timer;
	timer.Start();

	uint64_t nodeCount = 0;
	Position position;

	for (size_t i = 0; i < benchMarkPositions.size(); i++)
	{
		if (!position.InitialiseFromFen(benchMarkPositions[i]))
		{
			cout << "BAD FEN!" << endl;
			break;
		}

		uint64_t nodes = BenchSearch(position, 8);
		nodeCount += nodes;
	}

	cout << nodeCount << " nodes " << int(nodeCount / max(timer.ElapsedMs(), 1) * 1000) << " nps" << endl;
}
