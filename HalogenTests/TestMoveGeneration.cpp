#include "CppUnitTest.h"

#include "..\Halogen\src\Search.h"

#include "..\Halogen\src\MoveList.cpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MoveGeneration
{
	uint64_t PerftCalculate(unsigned int depth, Position& position)
	{
		if (depth == 0)
			return 1;	//if perftdivide is called with 1 this is necesary

		uint64_t nodeCount = 0;
		ExtendedMoveList moves;
		moves.clear();
		LegalMoves(position, moves);

		if (depth == 1)
			return moves.size();

		for (size_t i = 0; i < moves.size(); i++)
		{
			position.ApplyMove(moves[i].move);
			nodeCount += PerftCalculate(depth - 1, position);
			position.RevertMove();
		}

		return nodeCount;
	}

	TEST_CLASS(Perft)
	{
	public:
		Perft()
		{
			ZobristInit();
			BBInit();
			Network::Init();
		}

		TEST_METHOD(Position1)
		{
			Position position;
			position.InitialiseFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
			Assert::AreEqual<uint64_t>(4865609, PerftCalculate(5, position));
		}

		TEST_METHOD(Position2)
		{
			Position position;
			position.InitialiseFromFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
			Assert::AreEqual<uint64_t>(4085603, PerftCalculate(4, position));
		}

		TEST_METHOD(Position3)
		{
			Position position;
			position.InitialiseFromFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ");
			Assert::AreEqual<uint64_t>(674624, PerftCalculate(5, position));
		}

		TEST_METHOD(Position4)
		{
			Position position;
			position.InitialiseFromFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
			Assert::AreEqual<uint64_t>(422333, PerftCalculate(4, position));
		}

		TEST_METHOD(Position5)
		{
			Position position;
			position.InitialiseFromFen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ");
			Assert::AreEqual<uint64_t>(2103487, PerftCalculate(4, position));
		}

		TEST_METHOD(Position6)
		{
			Position position;
			position.InitialiseFromFen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ");
			Assert::AreEqual<uint64_t>(3894594, PerftCalculate(4, position));
		}
	};
}
