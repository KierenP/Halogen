#include "Search.h"
#include "PerftTT.h"
#include <string>
#include <sstream>
#include <fstream>
#include <ctime>
#include <Windows.h>

using namespace::std;
Position GameBoard;
PerftTT PerftTable;

//Move Search(float time);		//time remaining in milliseconds
void Benchmark();
void PerftSuite();
void PrintSearchInfo(ABnode root, unsigned int depth, double Time);
unsigned int PerftDivide(unsigned int depth);
unsigned int Perft(unsigned int depth);

double operator-(const SYSTEMTIME& pSr, const SYSTEMTIME& pSl);

int main()
{
	std::cout << "Version 2.2" << std::endl;

	unsigned long long init[4] = { 0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL }, length = 4;
	init_by_array64(init, length);

	srand(time(NULL));
	BBInit();

	string Line;					//to read the command given by the GUI
	cout.setf(ios::unitbuf);		// Make sure that the outputs are sent straight away to the GUI

	GameBoard.StartingPosition();
	ZobristInit();
	InitializeEvaluation();
	//GameBoard.InitialiseFromFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
	//EvaluatePosition(GameBoard);
	//PerftSuite();
	//Benchmark();
	//SearchPosition(GameBoard, 1500000000, true);

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
			cout << "id name Halogen 2" << endl;
			cout << "id author Kieren Pearson" << endl;
			cout << "uciok" << endl;
		}
		else if (Line == "isready") {
			cout << "readyok" << endl;
		}
		else if (Line == "ucinewgame") {
			GameBoard.StartingPosition();
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
	HANDLE  hConsole;
	SYSTEMTIME before;
	SYSTEMTIME after;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	unsigned int Score = 0;
	std::string line;

	GetSystemTime(&before);
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

		GetSystemTime(&before);
		Move BestMove = SearchPosition(GameBoard, 1000 * 120, true);																		
		GetSystemTime(&after);
		double Time = after.wDay * 1000 * 60 * 60 * 24 + after.wHour * 60 * 60 * 1000 + after.wMinute * 60 * 1000 + after.wSecond * 1000 + after.wMilliseconds - before.wDay * 1000 * 60 * 60 * 24 - before.wHour * 60 * 60 * 1000 - before.wMinute * 60 * 1000 - before.wSecond * 1000 - before.wMilliseconds;

		unsigned int prev = BestMove.GetFrom();
		unsigned int current = BestMove.GetTo();

		//string moveStr = { char(prev % 8 + 97), prev / 8 + 1, char(current % 8 + 97), char(current / 8 + 1) };
		string moveStr;
		moveStr.push_back(char(prev % 8 + 97));
		moveStr.push_back('0' + prev / 8 + 1);
		moveStr.push_back(char(current % 8 + 97));
		moveStr.push_back('0' + current / 8 + 1);
		//char(prev % 8 + 97), prev / 8 + 1, current % 8 + 97), current / 8 + 1

		if (moveStr == arrayTokens[arrayTokens.size() - 3] || moveStr == arrayTokens[arrayTokens.size() - 2])
		{		//correct
			SetConsoleTextAttribute(hConsole, 2);	//green text
			std::cout << "CORRECT bestmove " << moveStr;
			std::cout << " in: " << Time / 1000 << "s";
			SetConsoleTextAttribute(hConsole, 7);	//green text
			Score++;
		}
		else
		{
			SetConsoleTextAttribute(hConsole, 4);	//green text
			std::cout << "INCORRECT bestmove " << moveStr;
			std::cout << " in: " << Time / 1000 << "s correct move was: " << arrayTokens[arrayTokens.size() - 2];
			SetConsoleTextAttribute(hConsole, 7);	//green text
		}
	}

	std::cout << "\n\nCompleted all positions with score: " << Score << "/24";
}

void PerftSuite()
{
	std::ifstream infile("perftsuite.txt");
	HANDLE  hConsole;
	SYSTEMTIME before;
	SYSTEMTIME after;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	unsigned int Perfts = 0;
	unsigned int Correct = 0;
	double Totalnodes = 0;

	std::string line;

	GetSystemTime(&before);
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
	GetSystemTime(&after);

	double Time = after.wDay * 1000 * 60 * 60 * 24 + after.wHour * 60 * 60 * 1000 + after.wMinute * 60 * 1000 + after.wSecond * 1000 + after.wMilliseconds - before.wDay * 1000 * 60 * 60 * 24 - before.wHour * 60 * 60 * 1000 - before.wMinute * 60 * 1000 - before.wSecond * 1000 - before.wMilliseconds;

	std::cout << "\n\nCompleted perft with: " << Correct << "/" << Perfts << " correct";
	std::cout << "\nTotal nodes: " << (Totalnodes) << " in " << (Time / 1000) << "s";
	std::cout << "\nNodes per second: " << static_cast<unsigned int>((Totalnodes / Time) * 1000);
}

void PrintSearchInfo(ABnode root, unsigned int depth, double Time)
{
	if (root.GetScore() > -5000 && root.GetScore() < 5000)
	{
		std::cout
			<< "info depth " << depth
			<< " seldepth " << root.CountNodeChildren()
			<< " score cp " << (GameBoard.GetTurn() * 2 - 1) * root.GetScore()
			<< " time " << Time
			<< " nodes " << TotalNodeCount
			<< " nps " << TotalNodeCount / (Time / 1000)
			<< " tbhits " << tTable.GetCount()
			<< " pv ";
	}
	else
	{
		std::cout
			<< "info depth " << depth
			<< " seldepth " << root.CountNodeChildren()
			<< " score mate " << (root.CountNodeChildren() + 1) / 2
			<< " time " << Time
			<< " nodes " << TotalNodeCount
			<< " nps " << TotalNodeCount / (Time / 1000)
			<< " tbhits " << tTable.GetCount()
			<< " pv ";
	}

	for (ABnode* ptr = &root; ptr->HasChild(); ptr = ptr->GetChild())
	{
		ptr->GetMove().Print();
		std::cout << " ";
	}

	std::cout << std::endl;
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
	//OrderMoves(moves, GameBoard, 5);

	//GameBoard.ApplyNullMove();
	//GameBoard.RevertNullMove();

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

double operator-(const SYSTEMTIME& pSr, const SYSTEMTIME& pSl)
{
	FILETIME v_ftime;
	ULARGE_INTEGER v_ui;
	__int64 v_right, v_left;
	SystemTimeToFileTime(&pSr, &v_ftime);
	v_ui.LowPart = v_ftime.dwLowDateTime;
	v_ui.HighPart = v_ftime.dwHighDateTime;
	v_right = v_ui.QuadPart;

	SystemTimeToFileTime(&pSl, &v_ftime);
	v_ui.LowPart = v_ftime.dwLowDateTime;
	v_ui.HighPart = v_ftime.dwHighDateTime;
	v_left = v_ui.QuadPart;

	return v_right - v_left;
}
