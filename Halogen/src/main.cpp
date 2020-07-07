#include "Benchmark.h"
#include "Search.h"
#include <thread>
#include <algorithm>
#include <random>

using namespace::std;
Position GameBoard;

void PerftSuite();
uint64_t PerftDivide(unsigned int depth, Position& position);
uint64_t Perft(unsigned int depth, Position& position);
void Bench(Position& position);
void Texel(std::vector<int*> params);

void PrintIteration(double error, std::vector<int*>& params, std::vector<double> paramiterValues, double step_size, int iteration);

double CalculateError(std::vector<std::pair<Position, double>>& positions, ThreadData& data, double k, unsigned int subset);

double CalculateK(std::vector<Position>& positionList, std::vector<double>& positionScore, std::vector<double>& positionResults);

string version = "4.3"; 

int main(int argc, char* argv[])
{
	std::cout << "Halogen " << version << std::endl;

	unsigned long long init[4] = { 0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL }, length = 4;
	init_by_array64(init, length);

	ZobristInit();
	BBInit();
	InitializeEvaluation();

	string Line;					//to read the command given by the GUI
	cout.setf(ios::unitbuf);		// Make sure that the outputs are sent straight away to the GUI
	GameBoard.StartingPosition();

	//EvaluateDebug();				//uncomment for debug purposes. Must be run in debug mode to work
	//PerftSuite();

	tTable.SetSize(1);
	pawnHashTable.Init(1);

	if (argc == 2 && strcmp(argv[1], "bench") == 0) { Bench(GameBoard); return 0; }	//currently only supports bench from command line for openBench integration

	while (getline(cin, Line))
	{
		std::istringstream iss(Line);
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

			std::thread searchThread([&] {MultithreadedSearch(GameBoard, movetime, ThreadCount); });
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
					std::cout << "info string Hash size too small" << std::endl;
				else if (size > 8192)
					std::cout << "info string Hash size too large" << std::endl;
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
					std::cout << "info string thread count too small" << std::endl;
				else if (size > 8)
					std::cout << "info string thread count too large" << std::endl;
				else
				{
					ThreadCount = size;
				}
			}
		}

		else if (token == "perft")
		{
			iss >> token;
			PerftDivide(stoi(token), GameBoard);
		}

		else if (token == "texel")
		{
			Texel(TexelParamiters());
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
	unsigned int prev = ThreadCount;
	ThreadCount = 1;

	Timer timer;
	timer.Start();

	uint64_t nodeCount = 0;

	for (int i = 0; i < benchMarkPositions.size(); i++)
	{
		if (!position.InitialiseFromFen(benchMarkPositions[i]))
		{
			std::cout << "BAD FEN!" << std::endl;
			break;
		}

		position.Print();

		uint64_t nodes = BenchSearch(position, 8);
		std::cout << std::endl;

		nodeCount += nodes;
	}

	std::cout << nodeCount << " nodes " << int(nodeCount / max(timer.ElapsedMs(), 1) * 1000) << " nps" << std::endl;
	ThreadCount = prev;
}

void Texel(std::vector<int*> params)
{
	/*
	Make sure you disable pawn hash tables and transposition tables!
	*/
	
	std::ifstream infile("quiet-labeled.epd");

	if (!infile) 
	{
		cout << "Cannot open position file!" << endl;
		return;
	}

	std::string line;
	int lineCount = 0;

	cout << "Beginning to read in positions..." << endl;

	std::vector<std::pair<Position, double>> positions;

	while (std::getline(infile, line))
	{
		if (lineCount % 10000 == 0)
			cout << "Reading line: " << lineCount << "\r";

		vector<string> arrayTokens;
		std::istringstream iss(line);
		arrayTokens.clear();

		do
		{
			std::string stub;
			iss >> stub;

			if (stub == "c9")
			{
				arrayTokens.push_back("0");
				arrayTokens.push_back("1");
				break;
			}

			arrayTokens.push_back(stub);
		} while (iss);

		if (!GameBoard.InitialiseFromFen(arrayTokens))
		{
			std::cout << "line " << lineCount + 1 << ": BAD FEN" << endl;
		}

		std::string result;
		iss >> result;
		
		if (result == "\"0-1\";") 
		{
			positions.push_back({ GameBoard, 0 });
		}
		else if (result == "\"1-0\";")
		{
			positions.push_back({ GameBoard, 1 });
		}
		else if (result == "\"1/2-1/2\";")
		{
			positions.push_back({ GameBoard, 0.5 });
		}
		else
		{
			std::cout << "line " << lineCount + 1 << ": Could not read result" << endl;
		}

		lineCount++;
	}

	cout << "All positions loaded successfully" << endl;
	cout << "\nEvaluating positions..." << endl;

	ThreadData data;
	uint64_t totalScore = 0;

	for (int i = 0; i < positions.size(); i++)
	{
		int score = TexelSearch(positions[i].first, data);
		totalScore += score;
	}

	cout << "All positions evaluated successfully" << endl;
	cout << "Eval hash: " << totalScore << endl;
	cout << "\nBeginning Texel tuning..." << endl;

	//CalculateK(positionList, positionScore, positionResults);
	double k = 1.51;
	double prevError = 0;
	double step_size = 100000;
	int iteration = 1;

	auto rng = std::default_random_engine{};

	std::vector<double> paramiterValues;

	for (int i = 0; i < params.size(); i++)
	{
		paramiterValues.push_back(*params[i]);
	}

	while (true)
	{
		std::shuffle(std::begin(positions), std::end(positions), rng);

		double error = CalculateError(positions, data, k, positions.size() / params.size());
		PrintIteration(error, params, paramiterValues, step_size, iteration);

		vector<double> gradient;

		for (int i = 0; i < params.size(); i++)
		{		
			(*params[i])++;
			double error_plus_epsilon = CalculateError(positions, data, k, positions.size() / params.size());
			(*params[i])--;
			double firstDerivitive = (error_plus_epsilon - error);			

			gradient.push_back(firstDerivitive);
		}

		for (int i = 0; i < params.size(); i++)
		{
			double delta = step_size * gradient[i];
			paramiterValues[i] -= delta;
			(*params[i]) = round(paramiterValues[i]);

			//delta = (delta < 0) ? floor(delta) : ceil(delta);		//round away from zero

			/*if (int(delta) == -1)	//error_plus_epsilon was worse, so we are going to go back by one. To prevent oscelations, check going back is strictly better
			{
				(*params[i])--;
				double error_minus_epsilon = CalculateError(positions, data, k, positions.size() / params.size());
				(*params[i])++;

				if (error_minus_epsilon > error)
					delta = 0;
			}*/
		}

		step_size *= 1 / (1 + 0.001 * iteration);
		iteration++;
	}

	cout << "Texel complete" << endl;

	return;
}

void PrintIteration(double error, std::vector<int*>& params, std::vector<double> paramiterValues, double step_size, int iteration)
{
	cout << "Error: " << error << endl;
	cout << "Step size: " << step_size << endl;
	cout << "Iteration: " << iteration << endl;

	for (int i = 0; i < params.size(); i++)
	{
		cout << "Paramiter " << i << ": " << *(params[i]) << " exact value: " << paramiterValues[i] << endl;
	}

	cout << endl;
}

double CalculateError(std::vector<std::pair<Position, double>>& positions, ThreadData& data, double k, unsigned int subset)
{
	double error = 0;
	for (int i = 0; i < subset; i++)
	{
		double sigmoid = 1 / (1 + pow(10, -TexelSearch(positions[i].first, data) * k / 400));
		error += pow(positions[i].second - sigmoid, 2);
	}
	error /= subset;
	return error;
}

void TexelOptimise()
{

}

double CalculateK(std::vector<Position>& positionList, std::vector<double>& positionScore, std::vector<double>& positionResults)
{
	double k = 0;
	double epsilon = 0.0001;

	for (int rep = 0; rep < 100; rep++)
	{
		double error_k = 0;
		double error_k_minus_epsilon = 0;
		double error_k_plus_epsilon = 0;
		double sigmoid = 0;

		for (int i = 0; i < positionList.size(); i++)
		{
			sigmoid = 1 / (1 + pow(10, -positionScore[i] * k / 400));
			error_k += pow(positionResults[i] - sigmoid, 2);

			sigmoid = 1 / (1 + pow(10, -positionScore[i] * (k + epsilon) / 400));
			error_k_plus_epsilon += pow(positionResults[i] - sigmoid, 2);

			sigmoid = 1 / (1 + pow(10, -positionScore[i] * (k - epsilon) / 400));
			error_k_minus_epsilon += pow(positionResults[i] - sigmoid, 2);
		}

		error_k /= positionList.size();
		error_k_minus_epsilon /= positionList.size();
		error_k_plus_epsilon /= positionList.size();

		cout << "evaluation error: " << error_k << " K: " << k << endl;

		double firstDerivitive = (error_k_plus_epsilon - error_k) / epsilon;
		double secondDerivitive = (error_k_plus_epsilon - 2 * error_k + error_k_minus_epsilon) / (epsilon * epsilon);

		double newk = k - firstDerivitive / secondDerivitive;

		if (abs(newk - k) < epsilon)
			return k;

		k = newk;
	}

	cout << "Failed to converge after 100 iterations" << endl;
	return k;
}
