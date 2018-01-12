#include "Position.h"
#include "Random.h"
#include "PerftTT.h"
#include <string>
#include <sstream>
#include <fstream>

using namespace::std;
Position GameBoard;

//const bool DEBUG = true;

Move Search(float time);		//time remaining in milliseconds
void Benchmark();
void PerftSuite();
string EncodeMove(Move move);

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
	//PerftSuite();
	//Benchmark();

	//std::cout << GameBoard.Evaluate();

	//GameBoard.InitialiseFromFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R", "w", "KQkq", "-", "0", "1");
	//GameBoard.InitialiseFromFen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1", "w", "-", "-", "0", "1");
	//GameBoard.Print();
	//SYSTEMTIME before;
	//SYSTEMTIME after;

	//GetSystemTime(&before);
	//double nodes = GameBoard.Perft(8);
	//GetSystemTime(&after);

	//double Time = after.wDay * 1000 * 60 * 60 * 24 + after.wHour * 60 * 60 * 1000 + after.wMinute * 60 * 1000 + after.wSecond * 1000 + after.wMilliseconds - before.wDay * 1000 * 60 * 60 * 24 - before.wHour * 60 * 60 * 1000 - before.wMinute * 60 * 1000 - before.wSecond * 1000 - before.wMilliseconds;

	//std::cout.precision(17);
	//std::cout << "\n\n Perft with depth " << 4 << " = " << nodes << " leaf nodes in: " << Time  << "ms at: " << static_cast<unsigned int>(nodes / Time * 1000) << " nps";
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
		if (Line == "uci") {
			cout << "id name Halogen 2" << endl;
			cout << "id author Kieren Pearson" << endl;
			cout << "uciok" << endl;
		}
		else if (Line == "quit") {
			cout << "Bye Bye" << endl;
			break;
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

		if (Line.substr(0, 24) == "position startpos moves ") {

			GameBoard.StartingPosition();
			vector<string> arrayTokens;
			std::istringstream iss(Line);
			arrayTokens.clear();

			do
			{
				std::string stub;
				iss >> stub;
				arrayTokens.push_back(stub);
			} while (iss);

			for (int i = 3; i < arrayTokens.size() - 1; i++)
			{
				GameBoard.ApplyMove(arrayTokens[i]);
			}

			//GameBoard.Print();
		}
		/*else if (Line.substr(0, 6) == "perft")
		{
			unsigned long int nodes = GameBoard.Divide(1);

			std::cout << "\n\n Perft with depth " << 1 << " = " << nodes << " leaf nodes";

			GameBoard.Print();
		}*/
		else if (Line.substr(0, 3) == "go ") {
			// Received a command like: "go wtime 300000 btime 300000 winc 0 binc 0"

			Move BestMove(0, 0, 0);
			vector<string> arrayTokens;
			std::istringstream iss(Line);
			arrayTokens.clear();

			do
			{
				std::string stub;
				iss >> stub;
				arrayTokens.push_back(stub);
			} while (iss);

			if (GameBoard.GetCurrentTurn() == WHITE)
				BestMove = Search(stoi(arrayTokens[2]));
			if (GameBoard.GetCurrentTurn() == BLACK)
				BestMove = Search(stoi(arrayTokens[4]));
			std::cout << "\nbestmove ";
			BestMove.Print();
			std::cout << std::endl;
			GameBoard.ApplyMove(BestMove);
		}
	}

	return 0;
}

Move Search(float time)
{
	GameBoard.ResetTT();
	TotalNodeCount = 0;

	SYSTEMTIME before;
	SYSTEMTIME after;
	
	double prevTime = 1;
	double TotalTime = 1;

	unsigned int DepthMultiRatio = 1;
	unsigned int DesiredTime = time / 20;	//try to aim to use up a 20th of the available time each turn	

	ABnode* ROOT = new ABnode(0, Move(0, 0, 0), 0, 0);
	Move bestmove(0, 0, 0);

	bool checkmate = false;
	int prevScore = 0;

	for (int depth = 1; (DesiredTime > prevTime * DepthMultiRatio || depth < 3) && (!checkmate && depth < 15); depth++)
	{
		//GameBoard.Print();

		delete ROOT;
		ROOT = new ABnode(0, Move(0, 0, 0), 0, 0);

		GetSystemTime(&before);

		int alpha = prevScore - 50;
		int beta = prevScore + 50;

		do
		{
			if (GameBoard.BestMove(depth, alpha, beta, ROOT, bestmove, DesiredTime - TotalTime) == -2)
				return bestmove;

			if (ROOT->GetCutoff() == NONE)
				std::cout << "NONE";

			if (ROOT->GetCutoff() == ALPHA_CUTOFF)
			{
				std::cout << " Fail low ";// << alpha << " beta: " << beta << " prevScore: " << prevScore;
				alpha -= prevScore;
				alpha *= 4;				//4x distance from prevscore
				alpha += prevScore;
			}
			if (ROOT->GetCutoff() == BETA_CUTOFF)
			{
				std::cout << " Fail High ";// << alpha << " beta: " << beta << " prevScore: " << prevScore;
				beta -= prevScore;
				beta *= 4;				//4x distance from prevscore
				beta += prevScore;
			}
		} while (ROOT->GetCutoff() == BETA_CUTOFF);

		GetSystemTime(&after);

		double Time = after.wDay * 1000 * 60 * 60 * 24 + after.wHour * 60 * 60 * 1000 + after.wMinute * 60 * 1000 + after.wSecond * 1000 + after.wMilliseconds - before.wDay * 1000 * 60 * 60 * 24 - before.wHour * 60 * 60 * 1000 - before.wMinute * 60 * 1000 - before.wSecond * 1000 - before.wMilliseconds;

		DepthMultiRatio = Time / TotalTime;
		prevTime = Time;
		TotalTime += Time;

		ABnode* ptr = ROOT->GetChild();
		int actualdepth = 1;

		while (ptr->HasChild())
		{
			ptr = ptr->GetChild();
			actualdepth++;
		}

		if (ROOT->GetChild()->GetScore() > -9000 && ROOT->GetChild()->GetScore() < 9000)
		{
			std::cout << "\ninfo depth " << depth << " seldepth " << actualdepth << " score cp " << (GameBoard.GetCurrentTurn() * 2 - 1)* ROOT->GetChild()->GetScore() << " time " << Time << " nodes " << GameBoard.NodeCount << " nps " << GameBoard.NodeCount / (Time / 1000) << " tbhits " << GameBoard.tTable.TTHits << " pv ";
		}
		else
		{
			std::cout << "\ninfo depth " << depth << " seldepth " << actualdepth << " score mate " << (actualdepth + 1) / 2 << " time " << Time << " nodes " << GameBoard.NodeCount << " nps " << GameBoard.NodeCount / (Time / 1000) << " TBhits " << GameBoard.tTable.TTHits << " pv ";
			checkmate = true;
		}
		ROOT->GetChild()->GetMove().Print();
		std::cout << " ";

		ptr = ROOT->GetChild();

		while (ptr->HasChild())
		{
			ptr = ptr->GetChild();
			ptr->GetMove().Print();
			std::cout << " ";
		}

		bestmove = ROOT->GetChild()->GetMove();
		prevScore = ROOT->GetChild()->GetScore();
	}

	std::cout << "\n";
	delete ROOT;
	return bestmove;
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
		vector<string> arrayTokens;
		std::istringstream iss(line);
		arrayTokens.clear();

		do
		{
			std::string stub;
			iss >> stub;
			arrayTokens.push_back(stub);
		} while (iss);

		GameBoard.InitialiseFromFen(arrayTokens[0], arrayTokens[1], arrayTokens[2], arrayTokens[3], arrayTokens[4], arrayTokens[5]);
		//GameBoard.Print();
		
		unsigned int nodes = GameBoard.Perft((arrayTokens.size() - 7) / 2);
		if (nodes == stoi(arrayTokens[arrayTokens.size() - 2]))
		{
			SetConsoleTextAttribute(hConsole, 2);	//green text
			std::cout << "\nCORRECT Perft with depth " << (arrayTokens.size() - 6) / 2 << " = " << nodes << " leaf nodes";
			SetConsoleTextAttribute(hConsole, 7);	//back to gray
			Correct++;
		}
		else
		{
			SetConsoleTextAttribute(hConsole, 4);	//red text
			std::cout << "\nINCORRECT Perft with depth " << (arrayTokens.size() - 6) / 2 << " = " << nodes << " leaf nodes";
			SetConsoleTextAttribute(hConsole, 7);	//back to gray
		}

		Totalnodes += nodes;
		Perfts++;
	}
	GetSystemTime(&after);

	double Time = after.wDay * 1000 * 60 * 60 * 24 + after.wHour * 60 * 60 * 1000 + after.wMinute * 60 * 1000 + after.wSecond * 1000 + after.wMilliseconds - before.wDay * 1000 * 60 * 60 * 24 - before.wHour * 60 * 60 * 1000 - before.wMinute * 60 * 1000 - before.wSecond * 1000 - before.wMilliseconds;

	std::cout << "\n\nCompleted perft with: " << Correct << "/" << Perfts << " correct";
	std::cout << "\nTotal nodes: " << (Totalnodes / 1000) << " in " << (Time / 1000) << "s";
	std::cout << "\nNodes per second: " << static_cast<unsigned int>((Totalnodes / Time) * 1000);
}

string EncodeMove(Move move)
{
	/*unsigned int prev = move.GetFrom();
	unsigned int current = move.GetTo();

	//std::cout << (char)(prev % 8 + 97) << prev / 8 + 1 << (char)(current % 8 + 97) << current / 8 + 1;	//+1 to make it from 1-8 and not 0-7, 97 is ascii for 'a'

	if (GameBoard.GetSquare(move.GetFrom()) == WHITE_PAWN || GameBoard.GetSquare(move.GetFrom()) == BLACK_PAWN)		//pawn move
	{
		if (move.GetFlag() == QUIET || move.GetFlag() == PAWN_DOUBLE_MOVE)											//not a capture
			return string((char)(current % 8 + 97), current / 8 + 1);
	}*/

	return "0";
}
