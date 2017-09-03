#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\LogicalAnalisysLib\Dataset.h"
#include <CppUnitTestAssert.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LogicalAnalysisLibTest
{
	TEST_CLASS(DatasetTests)
	{
	public:
		
        TEST_METHOD(LoadEx6)
        {

            Dataset ds;

            ds.Load("../ao.test/datasets/ex6.txt");

            Assert::IsTrue(ds.objects.size() == 12);
            Assert::IsTrue(ds.classes.size() == 2);


        }

        TEST_METHOD(LoadIris)
        {
            Dataset ds;

            ds.Load("../ao.test/datasets/iris.txt");

            Assert::IsTrue(ds.objects.size() == 77);
            Assert::IsTrue(ds.classes.size() == 3);
        }

	};
}