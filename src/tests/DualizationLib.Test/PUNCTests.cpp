#include "stdafx.h"
#include "CppUnitTest.h"
#include "Counters.h"
#include "PUNC.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DualizationLibTest
{
	TEST_CLASS(PUNCTests)
	{
	public:
		
        void DoTest(std::string const& filename, int count)
        {
            PUNC::GlobalState A;
            A.LoadMatrix(filename);
            Counters c;
            A.CoverCallback = &c;
            A.Dualize();
            CHECK(c.cover == count);
        }

        TEST_CASE("DualizePUNCOrderedTest1)
        {
            PUNC::GlobalState A;
            A.LoadMatrix("../ao.test/datasets/10_25_480.hg");
            Counters c;
            A.CoverCallback = &c;

            Weights cw(25);
            for (int j = 0; j < 25; ++j)
                cw[j] = sin((Weight)j);

            Weights rw(10);
            for (int j = 0; j < 10; ++j)
                rw[j] = cos((Weight)j);

            A.SetRowsOrder(rw);
            A.SetColsOrder(std::move(cw));

            A.Dualize();
            CHECK(c.cover == 480);

        }

        TEST_CASE("DualizePUNCTest1)
        {
            DoTest("../ao.test/datasets/6_6_4.hg", 4);    
        }

        TEST_CASE("DualizePUNCTest2)
        {
            DoTest("../ao.test/datasets/13_7_10.hg", 10);
        }

        TEST_CASE("DualizePUNCTest3)
        {
            DoTest("../ao.test/datasets/15_10_14.hg", 14);            
        }

        TEST_CASE("DualizePUNCTest4)
        {
            DoTest("../ao.test/datasets/15_20_275.hg", 275);            
        }

        TEST_CASE("DualizePUNCTest5)
        {
            DoTest("../ao.test/datasets/10_25_480.hg", 480);    
        }

        TEST_CASE("DualizePUNCTest6)
        {
            DoTest("../ao.test/datasets/10_50_7187.hg", 7187);        
        }

        TEST_CASE("DualizePUNCTest7)
        {
            DoTest("../ao.test/datasets/15_35_12226.hg", 12226);    
        }
	};
}