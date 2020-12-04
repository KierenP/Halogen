#include "Benchmark.h"
#include "Search.h"
#include <thread>

using namespace::std; 

void PerftSuite();
void PrintVersion();
uint64_t PerftDivide(unsigned int depth, Position& position);
uint64_t Perft(unsigned int depth, Position& position);
void Bench();

string version = "8.1";  

void TestSyzygy();

int main(int argc, char* argv[])
{
	srand(time(NULL));

	PrintVersion();
	tb_init("<empty>");

	unsigned long long init[4] = { 0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL }, length = 4;
	init_by_array64(init, length);

	ZobristInit();
	BBInit();
	NetworkInit();

	string Line;					//to read the command given by the GUI
	cout.setf(ios::unitbuf);		// Make sure that the outputs are sent straight away to the GUI

	//EvaluateDebug();				//uncomment for debug purposes. Must be run in debug mode to work
	//PerftSuite();

	tTable.SetSize(1);

	Position position;
	thread searchThread;

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
			cout << "option name Hash type spin default 2 min 2 max 262144" << endl;
			cout << "option name Threads type spin default 1 min 1 max 256" << endl;
			cout << "option name SyzygyPath type string default <empty>" << endl;
			cout << "uciok" << endl;
		}

		else if (token == "isready") cout << "readyok" << endl;

		else if (token == "ucinewgame")
		{
			position.StartingPosition();
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
			int depth = 0;
			int mate = 0;

			while (iss >> token)
			{
				if (token == "wtime")	iss >> wtime;
				else if (token == "btime")	iss >> btime;
				else if (token == "winc")	iss >> winc;
				else if (token == "binc")	iss >> binc;
				else if (token == "movetime") iss >> searchTime;
				else if (token == "infinite") searchTime = 2147483647;
				else if (token == "movestogo") iss >> movestogo;
				else if (token == "depth") iss >> depth;
				else if (token == "mate") iss >> mate;
			}

			if (searchThread.joinable())
				searchThread.join();

			int myTime = position.GetTurn() ? wtime : btime;
			int myInc  = position.GetTurn() ? winc : binc;

			if (mate != 0)
				searchThread = thread([=, &position] {MateSearch(position, searchTime, mate); });
			else if (depth != 0) 										
				searchThread = thread([=, &position] {DepthSearch(position, depth); });															//fixed depth search
			else if (searchTime != 0) 							
				searchThread = thread([=, &position] {MultithreadedSearch(position, searchTime, searchTime, ThreadCount); });					//fixed time search
			else if (movestogo != 0)		
				searchThread = thread([=, &position] {MultithreadedSearch(position, myTime, myTime / (movestogo + 1) * 3 / 2, ThreadCount); });	//repeating time control
			else if (myInc != 0)
				searchThread = thread([=, &position] {MultithreadedSearch(position, myTime, myTime / 16 + myInc, ThreadCount); });				//increment time control
			else 
				searchThread = thread([=, &position] {MultithreadedSearch(position, myTime, myTime / 20, ThreadCount); });						//sudden death time control
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
				ThreadCount = stoi(token);
			}

			else if (token == "SyzygyPath")
			{
				iss >> token; //'value'
				iss >> token;

				tb_init(token.c_str());
				TestSyzygy();
			}

			else if (token == "LMR_constant")
			{
				iss >> token; //'value'
				iss >> token;
				LMR_constant = stod(token);
			}

			else if (token == "LMR_coeff")
			{
				iss >> token; //'value'
				iss >> token;
				LMR_coeff = stod(token);
			}
		}

		else if (token == "perft")
		{
			iss >> token;
			PerftDivide(stoi(token), position);
		}

		else if (token == "stop") 
		{
			KeepSearching = false;
			if (searchThread.joinable()) searchThread.join();	
		}

		else if (token == "quit") 
		{
			KeepSearching = false;
			if (searchThread.joinable()) searchThread.join();
			return 0;
		}

		//Non uci commands
		else if (token == "print") position.Print();
		else if (token == "bench") Bench();
		else cout << "Unknown command" << endl;
	}

	return 0;
}

void PrintVersion()
{
	cout << "Halogen " << version;

#if defined(_WIN64) or defined(__x86_64__)
	cout << " x64";

	#if defined(USE_POPCNT)
		cout << " POPCNT";
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

void TestSyzygy()
{
#ifdef DEBUG
	Position testPosition;
	testPosition.InitialiseFromFen("8/6B1/8/8/B7/8/K1pk4/8 b - - 0 1");
	unsigned int result = tb_probe_wdl(testPosition.GetWhitePieces(), testPosition.GetBlackPieces(),
		testPosition.GetPieceBB(WHITE_KING) | testPosition.GetPieceBB(BLACK_KING),
		testPosition.GetPieceBB(WHITE_QUEEN) | testPosition.GetPieceBB(BLACK_QUEEN),
		testPosition.GetPieceBB(WHITE_ROOK) | testPosition.GetPieceBB(BLACK_ROOK),
		testPosition.GetPieceBB(WHITE_BISHOP) | testPosition.GetPieceBB(BLACK_BISHOP),
		testPosition.GetPieceBB(WHITE_KNIGHT) | testPosition.GetPieceBB(BLACK_KNIGHT),
		testPosition.GetPieceBB(WHITE_PAWN) | testPosition.GetPieceBB(BLACK_PAWN),
		testPosition.GetFiftyMoveCount(),
		testPosition.CanCastleBlackKingside() * TB_CASTLING_k + testPosition.CanCastleBlackQueenside() * TB_CASTLING_q + testPosition.CanCastleWhiteKingside() * TB_CASTLING_K + testPosition.CanCastleWhiteQueenside() * TB_CASTLING_Q,
		testPosition.GetEnPassant() <= SQ_H8 ? testPosition.GetEnPassant() : 0,
		testPosition.GetTurn());
	assert(result == TB_BLESSED_LOSS);

	result = tb_probe_root(testPosition.GetWhitePieces(), testPosition.GetBlackPieces(),
		testPosition.GetPieceBB(WHITE_KING) | testPosition.GetPieceBB(BLACK_KING),
		testPosition.GetPieceBB(WHITE_QUEEN) | testPosition.GetPieceBB(BLACK_QUEEN),
		testPosition.GetPieceBB(WHITE_ROOK) | testPosition.GetPieceBB(BLACK_ROOK),
		testPosition.GetPieceBB(WHITE_BISHOP) | testPosition.GetPieceBB(BLACK_BISHOP),
		testPosition.GetPieceBB(WHITE_KNIGHT) | testPosition.GetPieceBB(BLACK_KNIGHT),
		testPosition.GetPieceBB(WHITE_PAWN) | testPosition.GetPieceBB(BLACK_PAWN),
		testPosition.GetFiftyMoveCount(),
		testPosition.CanCastleBlackKingside() * TB_CASTLING_k + testPosition.CanCastleBlackQueenside() * TB_CASTLING_q + testPosition.CanCastleWhiteKingside() * TB_CASTLING_K + testPosition.CanCastleWhiteQueenside() * TB_CASTLING_Q,
		testPosition.GetEnPassant() <= SQ_H8 ? testPosition.GetEnPassant() : 0,
		testPosition.GetTurn(),
		NULL);

	assert(TB_GET_WDL(result) == TB_BLESSED_LOSS);
	assert(TB_GET_FROM(result) == SQ_C2);
	assert(TB_GET_TO(result) == SQ_C1);
	assert(TB_GET_PROMOTES(result) == TB_PROMOTES_KNIGHT);
#endif 
}

void PerftSuite()
{
	ifstream infile("perftsuite.txt");

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
			cout << "\nCORRECT Perft with depth " << (arrayTokens.size() - 7) / 2 << " = " << nodes << " leaf nodes";
			Correct++;
		}
		else
		{
			cout << "\nINCORRECT Perft with depth " << (arrayTokens.size() - 7) / 2 << " = " << nodes << " leaf nodes";
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

	cout << "\nNodes searched: " << (nodeCount) << " in " << (elapsed_ms / 1000) << " seconds ";
	cout << "(" << static_cast<unsigned int>((nodeCount / elapsed_ms) * 1000) << " nps)" << endl;
	return nodeCount;
}

uint64_t Perft(unsigned int depth, Position& position)
{
	if (depth == 0)
		return 1;	//if perftdivide is called with 1 this is necesary

	uint64_t nodeCount = 0;
	vector<Move> moves;
	LegalMoves(position, moves);

	if (depth == 1)
		return moves.size();

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

		uint64_t nodes = BenchSearch(position, 12);
		nodeCount += nodes;
	}

	cout << nodeCount << " nodes " << int(nodeCount / max(timer.ElapsedMs(), 1) * 1000) << " nps" << endl;
}
