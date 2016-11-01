#include "stdafx.h"
#include "CppUnitTest.h"
#include "RUNC.h"
#include "Counters.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DualizationLibTest
{
	TEST_CLASS(RUNCTests)
	{
	public:
		
        
        void DoTest(std::string const& filename, int count)
        {
            Runc::AlgorithmType algs[] = {Runc::RUNC, Runc::OPT, Runc::RUNCM};

            for(auto a:algs)
            {
                Runc::GlobalState A(a);
                A.LoadMatrix(filename);
                Counters c;
                A.InnerCallback = &c;
                A.CoverCallback = &c;
                A.Dualize();
                Assert::AreEqual(count, c.cover);
            }
        }

        TEST_METHOD(DualizeRUNCTest1)
        {
            DoTest("../ao.test/datasets/6_6_4.hg", 4);    
        }

        TEST_METHOD(DualizeRUNCTest2)
        {
            DoTest("../ao.test/datasets/13_7_10.hg", 10);
        }

        TEST_METHOD(DualizeRUNCTest3)
        {
            DoTest("../ao.test/datasets/15_10_14.hg", 14);            
        }

        TEST_METHOD(DualizeRUNCTest4)
        {
            DoTest("../ao.test/datasets/15_20_275.hg", 275);            
        }

        TEST_METHOD(DualizeRUNCTest5)
        {
            DoTest("../ao.test/datasets/10_25_480.hg", 480);    
        }

        TEST_METHOD(DualizeRUNCTest6)
        {
            DoTest("../ao.test/datasets/10_50_7187.hg", 7187);        
        }

        TEST_METHOD(DualizeRUNCTest7)
        {
            DoTest("../ao.test/datasets/15_35_12226.hg", 12226);    
        }


	};
}