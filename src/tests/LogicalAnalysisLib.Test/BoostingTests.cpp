#include "stdafx.h"
#include "CppUnitTest.h"
#include "BoostingLearner.h"
#include "ECVotingLearner.h"

#include <boost/property_tree/info_parser.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LogicalAnalysisLibTest
{
	TEST_CLASS(BoostingTests)
	{
	public:
		
		TEST_CASE("ClassSelectionTest)
		{
			BoostingLearner bl;
            bl.CurrentOptions.iterationsCount = 3;

            bl.positiveLearner = std::make_shared<ECVotingLearner>();

            Dataset ds;
            ds.Load("../datasets/iris.int");

            auto cls = bl.Train(ds);
		}

        

	};
}