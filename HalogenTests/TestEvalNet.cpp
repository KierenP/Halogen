#include "CppUnitTest.h"

#include "..\Halogen\src\Search.h"

#include "..\Halogen\src\Bitboard.cpp"
#include "..\Halogen\src\BitBoardDefine.cpp"
#include "..\Halogen\src\BoardParameters.cpp"
#include "..\Halogen\src\EvalNet.cpp"
#include "..\Halogen\src\EvalCache.cpp"
#include "..\Halogen\src\Move.cpp"
#include "..\Halogen\src\MoveGeneration.cpp"
#include "..\Halogen\src\MoveGenerator.cpp"
#include "..\Halogen\src\Network.cpp"
#include "..\Halogen\src\Position.cpp"
#include "..\Halogen\src\Search.cpp"
#include "..\Halogen\src\SearchData.cpp"
#include "..\Halogen\src\TimeManage.cpp"
#include "..\Halogen\src\TranspositionTable.cpp"
#include "..\Halogen\src\TTEntry.cpp"
#include "..\Halogen\src\Zobrist.cpp"
#include "..\Halogen\src\Pyrrhic\tbprobe.cpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EvalNet
{
	using namespace UnitTestEvalNet;

	TEST_CLASS(tempoAdjustment)
	{
	public:
		tempoAdjustment()
		{
			ZobristInit();
		}

		TEST_METHOD(White)
		{
			Position position;
			position.InitialiseFromFen("k1n5/8/8/8/8/8/8/K2R4 w - - 0 1");

			int value = 100;
			TempoAdjustment(value, position);
			Assert::AreEqual(110, value);
		}

		TEST_METHOD(Black)
		{
			Position position;
			position.InitialiseFromFen("k7/8/8/8/8/8/8/K2R4 b - - 0 1");

			int value = 100;
			TempoAdjustment(value, position);
			Assert::AreEqual(90, value);
		}
	};

	TEST_CLASS(deadPosition)
	{
	public:
		deadPosition()
		{
			ZobristInit();
		}

		TEST_METHOD(KvK)
		{
			Position position;
			position.InitialiseFromFen("k7/8/8/8/8/8/8/K7 w - - 0 1");

			Assert::IsTrue(DeadPosition(position));
		}

		TEST_METHOD(KBvK)
		{
			Position position;
			position.InitialiseFromFen("k7/8/8/8/8/8/8/K2b4 w - - 0 1");

			Assert::IsTrue(DeadPosition(position));
		}

		TEST_METHOD(KNvK)
		{
			Position position;
			position.InitialiseFromFen("k7/8/8/8/8/8/8/K3N3 w - - 0 1");

			Assert::IsTrue(DeadPosition(position));
		}

		TEST_METHOD(KRvK)
		{
			Position position;
			position.InitialiseFromFen("k7/8/8/8/8/8/2r5/K7 w - - 0 1");

			Assert::IsFalse(DeadPosition(position));
		}
	};
}
