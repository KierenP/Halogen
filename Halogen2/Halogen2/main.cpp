#include "Search.h"
#include "PerftTT.h"
#include <string>
#include <sstream>
#include <fstream>

#include <Windows.h>

using namespace::std;
Position GameBoard;
PerftTT PerftTable;

void Benchmark();
void PerftSuite();
unsigned int PerftDivide(unsigned int depth);
unsigned int Perft(unsigned int depth);

int main()
{
	std::cout << "Version 2.4.0" << std::endl;

	unsigned long long init[4] = { 0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL }, length = 4;
	init_by_array64(init, length);

	srand(0);
	BBInit();

	string Line;					//to read the command given by the GUI
	cout.setf(ios::unitbuf);		// Make sure that the outputs are sent straight away to the GUI

	GameBoard.StartingPosition();
	ZobristInit();
	InitializeEvaluation();
	//GameBoard.InitialiseFromFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
	//GameBoard.InitialiseFromFen("r5k1/5ppp/1p6/p1p5/7b/1PPrqPP1/1PQ4P/R4R1K b - - 0 1");
	//GameBoard.InitialiseFromFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ");
	//GameBoard.InitialiseFromFen("8/5k2/7K/6P1/8/8/8/8 w - - 0 1");
	//GameBoard.InitialiseFromFen("7K/8/k1P5/7p/8/8/8/8 w - -");
	//GameBoard.InitialiseFromFen("8/7k/3b1K1B/8/p7/p7/R7/1b6 w - -");
	//SearchBenchmark(GameBoard, 9999999999, true);

	while (getline(cin, Line)) 
	{
		vector<string> arrayTokens;			//Split the line into an array of strings seperated by each space
		std::istringstream iss(Line);
		arrayTokens.clear();

		do
		{
			std::string stub;
			iss >> stub;
			arrayTokens.push_back(stub);
		} while (iss);

		if (arrayTokens.size() == 1)
		{
			std::cout << "Bad input: Empty input" << std::endl;
			continue;
		}

		if (Line == "uci") {
			cout << "id name Halogen 2.4.0" << endl;
			cout << "id author Kieren Pearson" << endl;
			cout << "uciok" << endl;
		}
		else if (Line == "isready") {
			cout << "readyok" << endl;
		}
		else if (Line == "ucinewgame") {
			GameBoard.StartingPosition();
			tTable.ResetTable(); //wipe the table
		}
		else if (Line == "quit") {
			return 0;
		}
		else if (arrayTokens[0] == "position")
		{
			if (arrayTokens[1] == "startpos")
			{
				GameBoard.StartingPosition();
				if (arrayTokens[2] == "moves")
				{
					GameBoard.InitialiseFromMoves(std::vector<std::string>(arrayTokens.begin() + 3, arrayTokens.end() - 1));
				}

				//GameBoard.Print();
			}
			else if (arrayTokens[1] == "fen")
			{
				if (arrayTokens.size() != 9)
				{
					std::cout << "Bad fen" << std::endl;
					continue;
				}

				GameBoard.InitialiseFromFen(arrayTokens[2], arrayTokens[3], arrayTokens[4], arrayTokens[5], arrayTokens[6], arrayTokens[7]);
			}
			else
			{
				std::cout << "Position input format not recognised" << std::endl;
				continue;
			}

			//GameBoard.Print();
			//EvaluatePosition(GameBoard);
		}
		else if (arrayTokens[0] == "go")
		{
			// Received a command like: "go wtime 300000 btime 300000 winc 0 binc 0"
			if (arrayTokens.size() != 10)
			{
				std::cout << "Bad go command" << std::endl;
				continue;
			}

			Move BestMove;
			unsigned int timeMs = stoi(GameBoard.GetTurn() == WHITE ? arrayTokens[2] : arrayTokens[4]);
			BestMove = SearchPosition(GameBoard, timeMs / 20, true);
			std::cout << std::endl;
			std::cout << "\nbestmove ";
			BestMove.Print();
			std::cout << std::endl;
			GameBoard.ApplyMove(BestMove);
		}
		else if (arrayTokens[0] == "analyse")
		{
			GameBoard.Print();
			Move BestMove;
			BestMove = SearchPosition(GameBoard, 9999999999, true);
		}
		else if (arrayTokens[0] == "PerftTest")
		{
			PerftSuite();
		}
		else if (arrayTokens[0] == "Perft")
		{
			int total = PerftDivide(stoi(arrayTokens[1]));
			std::cout << "\nTotal: " << total << std::endl;
		}
		else if (arrayTokens[0] == "Benchmark")
		{
			Benchmark();
		}
		else
		{
			std::cout << "Unknown command" << std::endl;
			continue;
		}
	}

	return 0;
}

void Benchmark()
{
	std::cout << "\nSTART\n";
	std::ifstream infile("bkt.txt");

	//For coloured text in console
	HANDLE  hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	float Score = 0;
	std::string line;

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

		GameBoard.InitialiseFromFen(arrayTokens[0], arrayTokens[1], arrayTokens[2], arrayTokens[3], "0", "1");
		GameBoard.Print();

		clock_t before = clock();
		std::vector<Move> BestMoves = SearchBenchmark(GameBoard, 1000 * 20, true);																		
		clock_t after = clock();

		double elapsed_ms = (double(after) - double(before)) / CLOCKS_PER_SEC * 1000;

		for (int i = 0; i < 4; i++)
		{
			unsigned int prev = BestMoves[i].GetFrom();
			unsigned int current = BestMoves[i].GetTo();

			string moveStr;
			moveStr.push_back(char(prev % 8 + 97));
			moveStr.push_back('0' + prev / 8 + 1);
			moveStr.push_back(char(current % 8 + 97));
			moveStr.push_back('0' + current / 8 + 1);

			if (moveStr == arrayTokens[arrayTokens.size() - 3] || moveStr == arrayTokens[arrayTokens.size() - 2])
			{		//correct

				if (i == 0)
					SetConsoleTextAttribute(hConsole, 10);	//green text
				else
					SetConsoleTextAttribute(hConsole, 9);	//yellow text

				std::cout << "CORRECT bestmove " << moveStr << " Rank: " << i + 1;
				std::cout << " in: " << elapsed_ms / 1000 << "s " << "Score: " << double(1) / (pow(2, i));
				SetConsoleTextAttribute(hConsole, 7);	//white text
				Score += 1 / (pow(2, i));
				break;
			}	
			else
			{
				if (i == 3)
				{
					SetConsoleTextAttribute(hConsole, 4);	//red text
					std::cout << "INCORRECT";
					std::cout << " in: " << elapsed_ms / 1000 << "s correct move was: " << arrayTokens[arrayTokens.size() - 2];
					SetConsoleTextAttribute(hConsole, 7);	//white text
				}
			}
		}
	}

	std::cout << "\n\nCompleted all positions with score: " << Score << "/24";
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
		PerftTable.Reformat();
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
		if (nodes == stoi(arrayTokens[arrayTokens.size() - 2]))
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
		GameBoard.ApplyMove(moves[i]);
		unsigned int ChildNodeCount = Perft(depth - 1);
		GameBoard.RevertMove(moves[i]);

		moves[i].Print();
		std::cout << ": " << ChildNodeCount << std::endl;
		nodeCount += ChildNodeCount;
	}

	return nodeCount;
}

unsigned int Perft(unsigned int depth)
{
	uint64_t key = GenerateZobristKey(GameBoard);
	if ((PerftTable.CheckEntry(key)) && (PerftTable.GetEntry(key).GetDepth() == depth))
		return PerftTable.GetEntry(key).GetNodes();

	unsigned int nodeCount = 0;
	std::vector<Move> moves = GenerateLegalMoves(GameBoard);

	if (depth == 1)
		return moves.size();

	for (int i = 0; i < moves.size(); i++)
	{
		GameBoard.ApplyMove(moves[i]);
		nodeCount += Perft(depth - 1);
		GameBoard.RevertMove(moves[i]);
	}

	PerftTable.AddEntry(key, nodeCount, depth);
	return nodeCount;
}
