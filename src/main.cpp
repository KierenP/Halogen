#include "Benchmark.h"
#include "Search.h"
#include <fstream>

using namespace::std;

void PerftSuite();
void PrintVersion();
uint64_t PerftDivide(unsigned int depth, Position& position);
uint64_t Perft(unsigned int depth, Position& position);
void Bench(int depth = 16);

string version = "10.16";

void RetrogradeSplit(std::string input);
void SyzygyLabel(std::string syzyzy_path, std::string input, std::string output);
void RetrogradePlayout(std::string input, int pieceCount, std::string output);
void Relabel(std::string input, std::string output);

int main(int argc, char* argv[])
{
	PrintVersion();
	tb_init("<empty>");

	ZobristInit();
	BBInit();
	Network::Init();

	string Line;					//to read the command given by the GUI
	cout.setf(ios::unitbuf);		// Make sure that the outputs are sent straight away to the GUI

	//PerftSuite();

	Position position;
	thread searchThread;
	SearchParameters parameters;

	for (int i = 1; i < argc; i++)	//read any command line input as a regular UCI instruction
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

			EvalCacheTable tmp;
			std::cout << "Eval: " << EvaluatePositionNet(position, tmp);
		}

		else if (token == "go")
		{
			SearchLimits limits;

			int wtime = 0;
			int btime = 0;
			int winc = 0;
			int binc = 0;
			int movestogo = 0;

			int searchTime = 0;
			int depth = 0;
			int mate = 0;

			while (iss >> token)
			{
				if (token == "wtime")	iss >> wtime;
				else if (token == "btime")	iss >> btime;
				else if (token == "winc")	iss >> winc;
				else if (token == "binc")	iss >> binc;
				else if (token == "movestogo") iss >> movestogo;

				else if (token == "mate")
				{
					iss >> mate;
					limits.SetMateLimit(mate);
				}

				else if (token == "depth")
				{
					iss >> depth;
					limits.SetDepthLimit(depth);
				}

				else if (token == "infinite")
				{
					limits.SetInfinite();
				}

				else if (token == "movetime")
				{
					iss >> searchTime;
					limits.SetTimeLimits(searchTime, searchTime);
				}
			}

			int myTime = position.GetTurn() ? wtime : btime;
			int myInc = position.GetTurn() ? winc : binc;

			if (myTime != 0)
			{
				int AllocatedTime = 0;

				if (movestogo != 0)
					AllocatedTime = myTime / (movestogo + 1) * 3 / 2;	//repeating time control
				else if (myInc != 0)
					// use a greater proportion of remaining time as the game continues, so that we use it all up and get to just increment
					AllocatedTime = myTime * (1 + position.GetTurnCount() / timeIncCoeffA) / timeIncCoeffB + myInc;	//increment time control
				else
					AllocatedTime = myTime / 20;						//sudden death time control

				limits.SetTimeLimits(myTime, AllocatedTime);
			}

			if (searchThread.joinable())
				searchThread.join();

			searchThread = thread([=] {SearchThread(position, parameters, limits); });
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
				parameters.threads = stoi(token);
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
				parameters.multiPV = stoi(token);
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

			else if (token == "Null_constant")
			{
				iss >> token; //'value'
				iss >> token;
				Null_constant = stoi(token);
			}

			else if (token == "Null_depth_quotent")
			{
				iss >> token; //'value'
				iss >> token;
				Null_depth_quotent = stoi(token);
			}

			else if (token == "Null_beta_quotent")
			{
				iss >> token; //'value'
				iss >> token;
				Null_beta_quotent = stoi(token);
			}

			else if (token == "Futility_constant")
			{
				iss >> token; //'value'
				iss >> token;
				Futility_constant = stoi(token);
			}

			else if (token == "Futility_coeff")
			{
				iss >> token; //'value'
				iss >> token;
				Futility_coeff = stoi(token);
			}

			else if (token == "Futility_depth")
			{
				iss >> token; //'value'
				iss >> token;
				Futility_depth = stoi(token);
			}

			else if (token == "Aspiration_window")
			{
				iss >> token; //'value'
				iss >> token;
				Aspiration_window = stoi(token);
			}

			else if (token == "Delta_margin")
			{
				iss >> token; //'value'
				iss >> token;
				Delta_margin = stoi(token);
			}

			else if (token == "SNMP_coeff")
			{
				iss >> token; //'value'
				iss >> token;
				SNMP_coeff = stoi(token);
			}

			else if (token == "SNMP_depth")
			{
				iss >> token; //'value'
				iss >> token;
				SNMP_depth = stoi(token);
			}

			else if (token == "LMP_constant")
			{
				iss >> token; //'value'
				iss >> token;
				LMP_constant = stoi(token);
			}

			else if (token == "LMP_coeff")
			{
				iss >> token; //'value'
				iss >> token;
				LMP_coeff = stoi(token);
			}

			else if (token == "LMP_depth")
			{
				iss >> token; //'value'
				iss >> token;
				LMP_depth = stoi(token);
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

		else if (token == "bench")
		{
			if (iss >> token)
				Bench(stoi(token));
			else
				Bench();
		}

		else if (token == "retrograde-split")
		{
			RetrogradeSplit(argv[2]);
		}

		else if (token == "syzygy-label")
		{
			SyzygyLabel(argv[2], argv[3], argv[4]);
		}

		else if (token == "retrograde-playout")
		{
			RetrogradePlayout(argv[2], stoi(argv[3]), argv[4]);
		}

		else if (token == "relabel")
		{
			Relabel(argv[2], argv[3]);
		}

		//Non uci commands
		else if (token == "print") position.Print();
		else cout << "Unknown command" << endl;

		Line.clear();

		if (argc != 1)	//Temporary fix to quit after a command line UCI argument is done
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
	BasicMoveList moves;
	LegalMoves(position, moves);

	for (size_t i = 0; i < moves.size(); i++)
	{
		position.ApplyMove(moves[i]);
		uint64_t ChildNodeCount = Perft(depth - 1, position);
		position.RevertMove();

		moves[i].Print();
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
	BasicMoveList moves;
	LegalMoves(position, moves);

	/*for (int i = 0; i < UINT16_MAX; i++)
	{
		Move test(i);
		bool legal = MoveIsLegal(position, test);

		bool present = false;

		for (size_t j = 0; j < moves.size(); j++)
		{
			if (moves[j].GetData() == i)
				present = true;
		}

		if (present != legal)
		{
			position.Print();
			test.Print();
			std::cout << std::endl;
			std::cout << present << " " << legal << std::endl;
			std::cout << test.GetFrom() << " " << test.GetTo() << " " << test.GetFlag() << std::endl;
			std::cout << MoveIsLegal(position, test);
		}
	}*/

	if (depth == 1)
		return moves.size();

	for (size_t i = 0; i < moves.size(); i++)
	{
		position.ApplyMove(moves[i]);
		nodeCount += Perft(depth - 1, position);
		position.RevertMove();
	}

	return nodeCount;
}

void Bench(int depth)
{
	Timer timer;
	timer.Start();

	uint64_t nodeCount = 0;
	Position position;
	SearchParameters parameters;

	for (size_t i = 0; i < benchMarkPositions.size(); i++)
	{
		if (!position.InitialiseFromFen(benchMarkPositions[i]))
		{
			cout << "BAD FEN!" << endl;
			break;
		}

		SearchLimits limits;
		limits.SetDepthLimit(depth);
		tTable.ResetTable();
		nodeCount += SearchThread(position, parameters, limits, false);
	}

	cout << nodeCount << " nodes " << int(nodeCount / max(timer.ElapsedMs(), 1) * 1000) << " nps" << endl;
}

//usage: Halogen.exe retrograde-split <source file>
void RetrogradeSplit(std::string input)
{
	std::ifstream source(input);

	if (!source.is_open())
	{
		std::cout << "Error, could not open input file\n";
		std::cout << "File name was: " << input << "\n";
		return;
	}

	Position position;
	vector<ofstream> outputFiles;

	for (int i = 0; i <= 32; i++)
	{
		outputFiles.emplace_back("piece_count_" + std::to_string(i) + ".fens");
	}

	std::string line;
	while (std::getline(source, line))
	{
		std::size_t last_word = line.find_last_of(' ');

		std::string fen = line.substr(0, last_word);
		std::string result = line.substr(last_word + 1);

		if (!position.InitialiseFromFen(fen))
		{
			std::cout << "Ignored bad fen " << fen << "\n";
			continue;
		}

		int pieces = GetBitCount(position.GetAllPieces());

		if (pieces > 32)
		{
			std::cout << "Somehow got more than 32 pieces??" << fen << "\n";
		}

		outputFiles[pieces] << fen << " [" << result << "]\n";
	}
}

//usage: Halogen.exe syzygy-label <syzyzy path> <source file> <output file>
void SyzygyLabel(std::string syzyzy_path, std::string input, std::string output)
{
	if (!tb_init(syzyzy_path.c_str()))
	{
		std::cout << "Error, could not load syzygy files\n";
		return;
	}

	std::ifstream source(input);

	if (!source.is_open())
	{
		std::cout << "Error, could not open input file\n";
		std::cout << "File name was: " << input << "\n";
		return;
	}

	std::ofstream dest(output, fstream::app);

	if (!dest.is_open())
	{
		std::cout << "Error, could not open output file\n";
		std::cout << "File name was: " << output << "\n";
		return;
	}

	Position position;

	std::string line;
	while (std::getline(source, line))
	{
		std::string fen = line;

		if (!position.InitialiseFromFen(fen))
		{
			std::cout << "Ignored bad fen " << fen << "\n";
			continue;
		}

		if (GetBitCount(position.GetAllPieces()) > TB_LARGEST
			|| position.GetCanCastleWhiteKingside() == true
			|| position.GetCanCastleBlackKingside() == true
			|| position.GetCanCastleWhiteQueenside() == true
			|| position.GetCanCastleBlackQueenside() == true)
		{
			std::cout << "Could not probe this position " << fen << "\n";
			continue;
		}

		unsigned int probe = tb_probe_root(position.GetWhitePieces(), position.GetBlackPieces(),
			position.GetPieceBB(WHITE_KING) | position.GetPieceBB(BLACK_KING),
			position.GetPieceBB(WHITE_QUEEN) | position.GetPieceBB(BLACK_QUEEN),
			position.GetPieceBB(WHITE_ROOK) | position.GetPieceBB(BLACK_ROOK),
			position.GetPieceBB(WHITE_BISHOP) | position.GetPieceBB(BLACK_BISHOP),
			position.GetPieceBB(WHITE_KNIGHT) | position.GetPieceBB(BLACK_KNIGHT),
			position.GetPieceBB(WHITE_PAWN) | position.GetPieceBB(BLACK_PAWN),
			position.GetFiftyMoveCount(),
			position.GetEnPassant() <= SQ_H8 ? position.GetEnPassant() : 0,
			position.GetTurn(),
			NULL);

		if (probe == TB_RESULT_FAILED ||
			probe == TB_RESULT_CHECKMATE ||
			probe == TB_RESULT_STALEMATE)
		{
			std::cout << "Position was terminal" << fen << "\n";
			continue;
		}

		int score = -1;

		if ((TB_GET_WDL(probe) == TB_LOSS && position.GetTurn() == WHITE) || (TB_GET_WDL(probe) == TB_WIN && position.GetTurn() == BLACK))
			score = TB_LOSS_SCORE;
		else if (TB_GET_WDL(probe) == TB_DRAW)
			score = 0;
		if ((TB_GET_WDL(probe) == TB_LOSS && position.GetTurn() == BLACK) || (TB_GET_WDL(probe) == TB_WIN && position.GetTurn() == WHITE))
			score = TB_WIN_SCORE;
		else
			assert(0);

		dest << fen << " " << score << "\n";
	}
}

#include <chrono>

void RetrogradePlayout(std::string input, int pieceCount, std::string output)
{
	std::ifstream source(input);

	if (!source.is_open())
	{
		std::cout << "Error, could not open input file\n";
		std::cout << "File name was: " << input << "\n";
		return;
	}

	std::ofstream dest(output, fstream::app);

	if (!dest.is_open())
	{
		std::cout << "Error, could not open output file\n";
		std::cout << "File name was: " << output << "\n";
		return;
	}

	Position position;

	int completed = 0;

	std::string line;
	while (std::getline(source, line))
	{
		std::string fen = line;

		if (!position.InitialiseFromFen(fen))
		{
			std::cout << "Ignored bad fen " << fen << "\n";
			continue;
		}

		KeepSearching = true;
		std::optional<int> score = PlayoutGame(position, pieceCount);

		if (!score)
			continue;

		dest << fen << " " << *score << "\n";
		completed++;
	}
}

void Relabel(std::string input, std::string output)
{
	std::ifstream source(input);

	if (!source.is_open())
	{
		std::cout << "Error, could not open input file\n";
		std::cout << "File name was: " << input << "\n";
		return;
	}

	std::ofstream dest(output, fstream::app);

	if (!dest.is_open())
	{
		std::cout << "Error, could not open output file\n";
		std::cout << "File name was: " << output << "\n";
		return;
	}

	Position position;
	EvalCacheTable tb;

	int completed = 0;

	std::string line;
	while (std::getline(source, line))
	{
		line = line.substr(0, line.find_last_of(" "));

		if (!position.InitialiseFromFen(line))
		{
			std::cout << "Ignored bad fen " << line << "\n";
			continue;
		}

		KeepSearching = true;
		int score = EvaluatePositionNet(position, tb);

		dest << line << " " << score << "\n";
		completed++;
	}
}