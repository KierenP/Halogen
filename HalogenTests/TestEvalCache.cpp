#include "CppUnitTest.h"

#include "..\Halogen\src\Search.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EvalCache
{
	TEST_CLASS(evalCacheTable)
	{
	public:

		EvalCacheTable table;

		TEST_METHOD(AddAndRetrieve)
		{
			table.AddEntry(0, 1024);
			int eval;
			Assert::IsTrue(table.GetEntry(0, eval));
			Assert::AreEqual(1024, eval);
		}

		TEST_METHOD(RetrieveNotThere)
		{
			int eval;
			Assert::IsFalse(table.GetEntry(1, eval));
		}

		TEST_METHOD(ClearTable)
		{
			table.AddEntry(2, 512);
			int eval;

			Assert::IsTrue(table.GetEntry(2, eval));
			table.Reset();
			Assert::IsFalse(table.GetEntry(2, eval));
		}
	};
}
