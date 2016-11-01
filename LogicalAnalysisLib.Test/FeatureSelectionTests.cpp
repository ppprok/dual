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
		
        TEST_METHOD(BoostingFeatureSelectorTest)
        {
            Dataset ds;
            ds.Load("../datasets/iris.int");
            
            InitECFeatureSelector bfs;
            bfs.Init(ds);
            
            for(auto& c : ds.classes)
            {
                Assert::IsFalse(
                    FeatureSelectorBase::AnyDefectPairs(bfs, c.positive, c.negative));
            }
        }

        TEST_METHOD(BoostingFeatureSelectorTest2)
        {
            Dataset ds;
            ds.Load("../datasets/mushrooms.int");

            InitECFeatureSelector bfs;
            bfs.Init(ds);

            for(auto& c : ds.classes)
            {
                Assert::IsFalse(
                    FeatureSelectorBase::AnyDefectPairs(bfs, c.positive, c.negative));
            }
        }

        TEST_METHOD(BinaryECFeatureSelectorTest2)
        {
            Dataset ds;
            ds.Load("../datasets/mushrooms.int");

            auto parts = ds.Split(0.5, 0.5);
            parts.first.CollectClasses();
            parts.second.CollectClasses();
            
            BinaryECFeatureSelector bfs;
            bfs.Init(ds);
            bfs.Select(parts.first.classes[0], parts.second.classes[0]);
                        
            
            Assert::IsFalse(
                FeatureSelectorBase::AnyDefectPairs(bfs, parts.first.classes[0].positive, parts.first.classes[0].negative));
            Assert::IsFalse(
                FeatureSelectorBase::AnyDefectPairs(bfs, parts.first.classes[1].positive, parts.first.classes[1].negative));
            
        }

        
	};
}