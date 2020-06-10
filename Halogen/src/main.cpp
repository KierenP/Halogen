#include "Search.h"
#include <thread>
#include <ppl.h>

using namespace::std;
Position GameBoard;

void PerftSuite();
uint64_t PerftDivide(unsigned int depth, Position& position);
uint64_t Perft(unsigned int depth, Position& position);
void Bench(Position& position);

string version = "3.8.2";
std::mutex Mutex;

int main()
{
	std::cout << version << std::endl;

	unsigned long long init[4] = { 0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL }, length = 4;
	init_by_array64(init, length);

	ZobristInit();
	BBInit();
	InitializeEvaluation();

	string Line;					//to read the command given by the GUI
	cout.setf(ios::unitbuf);		// Make sure that the outputs are sent straight away to the GUI
	GameBoard.StartingPosition();

	EvaluateDebug();				//uncomment for debug purposes. Must be run in debug mode to work
	//PerftSuite();

	tTable.SetSize(1);
	pawnHashTable.Init(1);

	//GameBoard.InitialiseFromFen("6k1/8/8/4QK2/8/8/8/8 w - - 10 6 ");
	//std::cout << GameBoard.GetZobristKey() << std::endl;

	while (getline(cin, Line))
	{
		std::istringstream iss(Line);
		string token;

		iss >> token;

		if (token == "uci")
		{
			cout << "id name Halogen" << version << " author Kieren Pearson" << endl;
			cout << "option name Clear Hash type button" << endl;
			cout << "option name Hash type spin default 2 min 2 max 8192" << endl;
			cout << "uciok" << endl;
		}

		else if (token == "isready") cout << "readyok" << endl;

		else if (token == "ucinewgame")
		{
			GameBoard.StartingPosition();
			pawnHashTable.ResetTable();
			tTable.ResetTable();
		}

		else if (token == "position")
		{
			GameBoard.Reset();
			GameBoard.StartingPosition();

			iss >> token;

			if (token == "fen")
			{
				vector<string> fen;

				while (iss >> token && token != "moves")
				{
					fen.push_back(token);
				}

				if (!GameBoard.InitialiseFromFen(fen)) cout << "BAD FEN";
				if (token == "moves") while (iss >> token) GameBoard.ApplyMove(token);
			}

			if (token == "startpos")
			{
				iss >> token;
				if (token == "moves") while (iss >> token) GameBoard.ApplyMove(token);
			}
		}

		else if (token == "go")
		{
			KeepSearching = true;

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

					if (GameBoard.GetTurn() == WHITE)
						movetime = wtime / 20 + winc;
					else
						movetime = btime / 20 + binc;
				}
				else
				{
					if (GameBoard.GetTurn() == WHITE)
						movetime = movestogo <= 1 ? wtime : wtime / (movestogo + 1) * 2;	
					else
						movetime = movestogo <= 1 ? btime : btime / (movestogo + 1) * 2;
				}
			}
			std::thread SearchThread(SearchPosition, GameBoard, movetime);
			SearchThread.detach();
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
					std::cout << "info string Hash size too small" << std::endl;
				else if (size > 8192)
					std::cout << "info string Hash size too large" << std::endl;
				else
				{
					tTable.SetSize(stoi(token) - 1);
					pawnHashTable.Init(1);
				}
			}
		}

		else if (token == "perft")
		{
			iss >> token;
			PerftDivide(stoi(token), GameBoard);
		}

		else if (token == "stop") KeepSearching = false;
		else if (token == "print") GameBoard.Print();
		else if (token == "quit") return 0;
		else if (token == "bench") Bench(GameBoard);
		else cout << "Unknown command" << endl;
	}

	return 0;
}

void PerftSuite()
{
	std::ifstream infile("perftsuite.txt");

	//multi-coloured text in concole 
	HANDLE  hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	unsigned int Perfts = 0;
	unsigned int Correct = 0;
	double Totalnodes = 0;

	std::string line;

	clock_t before = clock();
	while (std::getline(infile, line))
	{
		vector<string> arrayTokens;
		std::istringstream iss(line);
		arrayTokens.clear();

		do
		{
			std::string stub;
			iss >> stub;
			arrayTokens.push_back(stub);
		} while (iss);

		GameBoard.InitialiseFromFen(line);
		
		uint64_t nodes = Perft((arrayTokens.size() - 7) / 2, GameBoard);
		if (nodes == stoi(arrayTokens.at(arrayTokens.size() - 2)))
		{
			SetConsoleTextAttribute(hConsole, 2);	//green text
			std::cout << "\nCORRECT Perft with depth " << (arrayTokens.size() - 7) / 2 << " = " << nodes << " leaf nodes";
			SetConsoleTextAttribute(hConsole, 7);	//back to gray
			Correct++;
		}
		else
		{
			SetConsoleTextAttribute(hConsole, 4);	//red text
			std::cout << "\nINCORRECT Perft with depth " << (arrayTokens.size() - 7) / 2 << " = " << nodes << " leaf nodes";
			SetConsoleTextAttribute(hConsole, 7);	//back to gray
		}

		Totalnodes += nodes;
		Perfts++;
	}
	clock_t after = clock();

	double elapsed_ms = (double(after) - double(before)) / CLOCKS_PER_SEC * 1000;

	std::cout << "\n\nCompleted perft with: " << Correct << "/" << Perfts << " correct";
	std::cout << "\nTotal nodes: " << (Totalnodes) << " in " << (elapsed_ms / 1000) << "s";
	std::cout << "\nNodes per second: " << static_cast<unsigned int>((Totalnodes / elapsed_ms) * 1000);
}

uint64_t PerftDivide(unsigned int depth, Position& position)
{
	clock_t before = clock();

	uint64_t nodeCount = 0;
	std::vector<Move> moves;
	LegalMoves(GameBoard, moves);

	for (int i = 0; i < moves.size(); i++)
	{
		position.ApplyMove(moves.at(i));
		uint64_t ChildNodeCount = Perft(depth - 1, position);
		position.RevertMove();

		moves.at(i).Print();
		std::cout << ": " << ChildNodeCount << std::endl;
		nodeCount += ChildNodeCount;
	}

	clock_t after = clock();
	double elapsed_ms = (double(after) - double(before)) / CLOCKS_PER_SEC * 1000;

	std::cout << "\nTotal nodes: " << (nodeCount) << " in " << (elapsed_ms / 1000) << "s";
	std::cout << "\nNodes per second: " << static_cast<unsigned int>((nodeCount / elapsed_ms) * 1000);
	return nodeCount;
}

uint64_t Perft(unsigned int depth, Position& position)
{
	if (depth == 0)
		return 1;	//if perftdivide is called with 1 this is necesary

	uint64_t nodeCount = 0;
	std::vector<Move> moves;
	LegalMoves(position, moves);

	for (int i = 0; i < moves.size(); i++)
	{
		position.ApplyMove(moves.at(i));
		nodeCount += Perft(depth - 1, position);
		position.RevertMove();
	}

	return nodeCount;
}

void Bench(Position& position)
{
	std::cout << "Node Count: 1234" << std::endl;
	std::cout << "Nodes per second: 5678" << std::endl;
}
