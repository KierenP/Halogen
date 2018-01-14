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

Move Search(float time);		//time remaining in milliseconds
void Benchmark();
void PerftSuite();
void PrintSearchInfo(ABnode root, unsigned int depth, double Time);
unsigned int PerftDivide(unsigned int depth);
unsigned int Perft(unsigned int depth);

double operator-(const SYSTEMTIME& pSr, const SYSTEMTIME& pSl);

int main()
{
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
	//GameBoard.InitialiseFromFen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
	//EvaluatePosition(GameBoard);
	//GameBoard.InitialiseFromFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
	//EvaluatePosition(GameBoard);
	//GameBoard.InitialiseFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	//EvaluatePosition(GameBoard);
	PerftSuite();
	//Benchmark();

	//std::cout << GameBoard.Evaluate();
	//
	//std::cout << EvaluatePosition(GameBoard);
	//GameBoard.InitialiseFromFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
	//SYSTEMTIME before;
	//SYSTEMTIME after;

	////GameBoard.Print();
	//GetSystemTime(&before);
	//unsigned int nodes = PerftDivide(7);
	//GetSystemTime(&after);

	//double Time = after.wDay * 1000 * 60 * 60 * 24 + after.wHour * 60 * 60 * 1000 + after.wMinute * 60 * 1000 + after.wSecond * 1000 + after.wMilliseconds - before.wDay * 1000 * 60 * 60 * 24 - before.wHour * 60 * 60 * 1000 - before.wMinute * 60 * 1000 - before.wSecond * 1000 - before.wMilliseconds;

	////std::cout.precision(17);
	//std::cout << "\n\n Perft with depth " << 4 << " = " << nodes << " leaf nodes in: " << Time  << "ms at: " << static_cast<unsigned int>(nodes / Time * 1000) << " nps";
	//GameBoard.Print();
	//std::cin >> Line;

	//GameBoard.Evaluate();
	//Search(1500000000);
	//std::cout << "DONE";
	//std::cout << GameBoard.Perft(4);
	/*GameBoard.BestMove(1).GetMove().Print(); std::cout << "\n";
	GameBoard.BestMove(2).GetMove().Print(); std::cout << "\n";
	GameBoard.BestMove(3).GetMove().Print(); std::cout << "\n";
	GameBoard.BestMove(4).GetMove().Print(); std::cout << "\n";
	GameBoard.BestMove(5).GetMove().Print(); std::cout << "\n";
	GameBoard.BestMove(6).GetMove().Print(); std::cout << "\n";
	GameBoard.BestMove(7).GetMove().Print(); std::cout << "\n";
	GameBoard.BestMove(8).GetMove().Print(); std::cout << "\n";*/

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
			BestMove = Search(timeMs);
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

Move Search(float time)
{
	tTable.Reformat();
	TotalNodeCount = 0;

	SYSTEMTIME before;
	SYSTEMTIME after;
	double prevTime = 1;
	double TotalTime = 1;
	unsigned int DepthMultiRatio = 1;
	unsigned int DesiredTime = time / 20;	//try to aim to use up a 20th of the available time each turn	
	bool checkmate = false;
	ABnode Root;

	for (int depth = 1; (DesiredTime > prevTime * DepthMultiRatio || depth < 4) && (!checkmate); depth++)
	{
		GetSystemTime(&before);

		//int alpha = -99999;
		//int beta = prevScore + 25;

		/*do
		{
			if (GameBoard.BestMove(depth, alpha, beta, ROOT, bestmove, DesiredTime - TotalTime) == -1234567)
			{
				std::cout << std::endl;
				return bestmove;
			}

			if (ROOT->GetCutoff() == NONE)
				std::cout << "NONE";

			if (ROOT->GetChild()->GetScore() <= alpha)
			{
				//std::cout << " Fail low ";// << alpha << " beta: " << beta << " prevScore: " << prevScore;
				alpha -= prevScore;
				alpha *= 4;				//4x distance from prevscore
				alpha += prevScore;
			}

			if (ROOT->GetChild()->GetScore() >= beta)
			{
				//std::cout << " Fail High ";// << alpha << " beta: " << beta << " prevScore: " << prevScore;
				beta -= prevScore;
				beta *= 4;				//4x distance from prevscore
				beta += prevScore;
			}
		} while (ROOT->GetChild()->GetScore() <= alpha || ROOT->GetChild()->GetScore() >= beta);*/

		SearchBestMove(GameBoard, depth, Root.GetMove());
		GetSystemTime(&after);

		double Time = after.wDay * 1000 * 60 * 60 * 24 + after.wHour * 60 * 60 * 1000 + after.wMinute * 60 * 1000 + after.wSecond * 1000 + after.wMilliseconds - before.wDay * 1000 * 60 * 60 * 24 - before.wHour * 60 * 60 * 1000 - before.wMinute * 60 * 1000 - before.wSecond * 1000 - before.wMilliseconds;

		DepthMultiRatio = Time / TotalTime;
		prevTime = Time;
		TotalTime += Time;

		PrintSearchInfo(Root, depth, Time);
	}

	return Root.GetMove();
}

void Benchmark()
{
	/*std::cout << "\nSTART\n";
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

		GameBoard.InitialiseFromFen(arrayTokens[0], arrayTokens[1], arrayTokens[2], arrayTokens[3]);
		GameBoard.Print();

		GetSystemTime(&before);
		Move BestMove = Search(60 * 1000 * 2 * 20);																		//Give it 40 mins so it aims to solve in 2 mins
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

	std::cout << "\n\nCompleted all positions with score: " << Score << "/24";*/
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
			<< " seldepth " << root.TraverseNodeChildren()
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
			<< " seldepth " << root.TraverseNodeChildren()
			<< " score mate " << (root.TraverseNodeChildren() + 1) / 2
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
	PerftTable.Reformat();
	std::vector<Move> moves = GenerateLegalMoves(GameBoard);

	for (int i = 0; i < moves.size(); i++)
	{
		//PerftTable.Reformat();
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
