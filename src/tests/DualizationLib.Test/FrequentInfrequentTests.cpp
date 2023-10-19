#include "stdafx.h"
#include "CppUnitTest.h"
#include "FrequentInfrequentEnumerator.h"
#include "NumberChain.h"
#include "GraphPoset.h"
#include "IntervalLattice.h"
#include "ReversedPoset.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace PosetDualization;

namespace DualizationLibTest
{
	TEST_CLASS(FrequentInfrequentTests)
	{
	public:

        FrequentInfrequentTests()
        {
            NumberChain::Register();
            GraphPoset::Register();
            IntervalLattice::Register();
            ReversedPoset::Register();
        }
		
		TEST_CASE("FrequentInfrequentEnumeratorTest1)
		{
			FrequentInfrequentEnumerator enumerator;
            FrequentInfrequentTask task;

            
            auto opt = read_options("../dualizationlib.test/data/fif-task1.info");
            task.SetOptions(opt);
            task.LoadDatabase("../dualizationlib.test/data/A1.csv", opt);
            
            CHECK(std::vector<int>(1, 5) == task.GetThresholds());
            CHECK_EQ(16, task.GetDatabase().GetHeight());
            CHECK_EQ(3, task.GetDatabase().GetWidth());

            enumerator.Enumerate(task);
		}

	};
}