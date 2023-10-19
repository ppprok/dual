#include "stdafx.h"

#include <boost/property_tree/info_parser.hpp>

#include "CppUnitTest.h"
#include "..\LogicalAnalisysLib\InitECFeatureSelector.h"
#include "BinaryECFeatureSelector.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LogicalAnalysisLibTest
{
	TEST_CLASS(FeatureSelectionTests)
	{
	public:
		
        TEST_CASE("BoostingFeatureSelectorTest)
        {
            Dataset ds;
            ds.Load("../datasets/iris.int");
            
            InitECFeatureSelector bfs;
            bfs.Init(ds);
            
            for(auto& c : ds.classes)
            {
                CHECK_FALSE(
                    FeatureSelectorBase::AnyDefectPairs(bfs, c.positive, c.negative));
            }
        }

        TEST_CASE("BoostingFeatureSelectorTest2)
        {
            Dataset ds;
            ds.Load("../datasets/mushrooms.int");

            InitECFeatureSelector bfs;
            bfs.Init(ds);

            for(auto& c : ds.classes)
            {
                CHECK_FALSE(
                    FeatureSelectorBase::AnyDefectPairs(bfs, c.positive, c.negative));
            }
        }

        TEST_CASE("BinaryECFeatureSelectorTest2)
        {
            Dataset ds;
            ds.Load("../datasets/mushrooms.int");

            auto parts = ds.Split(0.5, 0.5);
            parts.first.CollectClasses();
            parts.second.CollectClasses();
            
            BinaryECFeatureSelector bfs;
            bfs.Init(ds);
            bfs.Select(parts.first.classes[0], parts.second.classes[0]);
                        
            
            CHECK_FALSE(
                FeatureSelectorBase::AnyDefectPairs(bfs, parts.first.classes[0].positive, parts.first.classes[0].negative));
            CHECK_FALSE(
                FeatureSelectorBase::AnyDefectPairs(bfs, parts.first.classes[1].positive, parts.first.classes[1].negative));
            
        }

        
	};
}