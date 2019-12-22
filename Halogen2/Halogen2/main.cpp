#include "Search.h"
#include <thread>

using namespace::std;
Position GameBoard;

void PerftSuite();
unsigned int PerftDivide(unsigned int depth);
unsigned int Perft(unsigned int depth);

string version = "2.7.3";

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

	EvaluateDebug();
	//PerftSuite();

	while (getline(cin, Line))
	{
		std::istringstream iss(Line);
		string token;

		iss >> token;

		if (token == "uci")
		{
			cout << "id name Halogen" << version << " author Kieren Pearson" << endl;
			cout << "option name Clear Hash type button" << endl;
			cout << "option name Hash type spin default 1 min 1 max 1024" << endl;
			cout << "uciok" << endl;
		}

		else if (token == "isready") cout << "readyok" << endl;

		else if (token == "ucinewgame")
		{
			GameBoard.StartingPosition();
			tTable.ResetTable();
		}

		else if (token == "position")
		{
			GameBoard.StartingPosition();
			PreviousKeys.clear();
			previousBoards.clear();
			PreviousParamiters.clear();

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
						movetime = wtime / 20;
					else
						movetime = btime / 20;
				}
				else
				{
					if (GameBoard.GetTurn() == WHITE)
						movetime = wtime / (movestogo) - 500 * (movestogo == 1);	//half a second buffer for if this is the last turn before extra time: we don't want to run the clock right down to zero!
					else
						movetime = btime / (movestogo) - 500 * (movestogo == 1);
				}
			}
			std::thread SearchThread(NegaMaxRoot, GameBoard, movetime);
			SearchThread.detach();
		}

		else if (token == "setoption")
		{
			iss >> token; //'name'
			iss >> token; 

			if (token == "Clear") 
			{
				iss >> token;
				if (token == "Hash") tTable.ResetTable();
			}

			else if (token == "Hash")
			{
				iss >> token; //'value'
				iss >> token;

				tTable.SetSize(stoi(token));
			}
		}

		else if (token == "stop") KeepSearching = false;
		else if (token == "quit") return 0;
		else if (token == "print") GameBoard.Print();
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
		
		unsigned int nodes = Perft((arrayTokens.size() - 7) / 2);
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

unsigned int PerftDivide(unsigned int depth)
{
	unsigned int nodeCount = 0;
	std::vector<Move> moves = GenerateLegalMoves(GameBoard);

	for (int i = 0; i < moves.size(); i++)
	{
		GameBoard.ApplyMove(moves.at(i));
		unsigned int ChildNodeCount = Perft(depth - 1);
		GameBoard.RevertMove(moves.at(i));

		moves.at(i).Print();
		std::cout << ": " << ChildNodeCount << std::endl;
		nodeCount += ChildNodeCount;
	}

	return nodeCount;
}

unsigned int Perft(unsigned int depth)
{
	unsigned int nodeCount = 0;
	std::vector<Move> moves = GenerateLegalMoves(GameBoard);

	if (depth == 1)
		return moves.size();

	for (int i = 0; i < moves.size(); i++)
	{
		GameBoard.ApplyMove(moves.at(i));
		nodeCount += Perft(depth - 1);
		GameBoard.RevertMove(moves.at(i));
	}

	return nodeCount;
}
