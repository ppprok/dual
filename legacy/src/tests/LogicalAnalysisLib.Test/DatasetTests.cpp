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
		
        TEST_CASE("LoadEx6)
        {

            Dataset ds;

            ds.Load("../ao.test/datasets/ex6.txt");

            CHECK(ds.objects.size() == 12);
            CHECK(ds.classes.size() == 2);


        }

        TEST_CASE("LoadIris)
        {
            Dataset ds;

            ds.Load("../ao.test/datasets/iris.txt");

            CHECK(ds.objects.size() == 77);
            CHECK(ds.classes.size() == 3);
        }

	};
}