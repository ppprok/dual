#include "stdafx.h"
#include "CppUnitTest.h"
#include "AOT1.h"
#include "Counters.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DualizationLibTest
{		
	TEST_CLASS(AOT1Tests)
	{
	public:
		
        

        void DoTest(std::string const& filename, int count)
        {
            AOT1::AlgorithmType algs[] = 
            {AOT1::AO1, AOT1::AO1M, AOT1::AO1K, AOT1::AO2, AOT1::AO2M, AOT1::AO2K};

            for(auto a:algs)
            {
                AOT1::GlobalState A(a);
                A.LoadMatrix(filename);
                Counters c;
                A.CoverCallback = &c;
                A.Dualize();
                Assert::IsTrue(c.cover == count);
            }
        }

        TEST_METHOD(DualizeAOT1Test1)
        {
            DoTest("../ao.test/datasets/6_6_4.hg", 4);    
        }

        TEST_METHOD(DualizeAOT1Test2)
        {
            DoTest("../ao.test/datasets/13_7_10.hg", 10);
        }

        TEST_METHOD(DualizeAOT1Test3)
        {
            DoTest("../ao.test/datasets/15_10_14.hg", 14);            
        }

        TEST_METHOD(DualizeAOT1Test4)
        {
            DoTest("../ao.test/datasets/15_20_275.hg", 275);            
        }

        TEST_METHOD(DualizeAOT1Test5)
        {
            DoTest("../ao.test/datasets/10_25_480.hg", 480);    
        }

        TEST_METHOD(DualizeAOT1Test6)
        {
            DoTest("../ao.test/datasets/10_50_7187.hg", 7187);        
        }

        TEST_METHOD(DualizeAOT1Test7)
        {
            DoTest("../ao.test/datasets/15_35_12226.hg", 12226);    
        }

	};
}