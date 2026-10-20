#include "stdafx.h"
#include "CppUnitTest.h"
#include "Dataset.h"
#include "FTVotingLearner.h"
#include "logging_utils.h"
#include "MultiByBinaryLearner.h"
#include "AlgorithmEstimator.h"
#include "BoostingLearner.h"
#include "..\LogicalAnalisysLib\ECmpByRankFeatureSelector.h"
#include "..\LogicalAnalisysLib\InitECFeatureSelector.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LogicalAnalysisLibTest
{		
    TEST_CLASS(ECmpVotingInitLBTests)
    {
    public:

        
        TEST_CASE("MbyBforIris)
        {
            auto e = CheckMultiByBinaryLearner("../datasets/iris.int");
            CHECK(e < 0.2);
        }

        TEST_CASE("MbyBforSoybean)
        {
            auto e = CheckMultiByBinaryLearner("../datasets/soybean-large.int");
            CHECK(e < 0.45);
        }

        TEST_CASE("MbyBforBalanceScale)
        {
            auto e = CheckMultiByBinaryLearner("../datasets/balance-scale.int");
            CHECK(e < 0.95);
        }

        TEST_CASE("BoostingForIris)
        {
            auto e = CheckBoostingLearner("../datasets/iris.int");
            CHECK(e == 0);
        }

        TEST_CASE("BoostingForSoybean)
        {
            auto e = CheckBoostingLearner("../datasets/soybean-large.int");
            CHECK(e == 0);
        }

/*        TEST_CASE("BoostingForBalanceScale)
        {
            auto e = CheckBoostingLearner("../datasets/balance-scale.int");
            CHECK(e == 0);
        }*/

        TEST_CASE("CVMbyBforIris)
        {
            auto e = CVMultiByBinaryLearner("../datasets/iris.int");
            CHECK(e < 0.2);
        }

/*        BEGIN_TEST_METHOD_ATTRIBUTE(CVMbyBforSoybean) 
            TEST_IGNORE()
            END_TEST_METHOD_ATTRIBUTE()*/
        TEST_CASE("CVMbyBforSoybean)
        {
            auto e = CVMultiByBinaryLearner("../datasets/soybean-large.int");
            CHECK(e < 0.3);
        }

        BEGIN_TEST_METHOD_ATTRIBUTE(CVMbyBforBalanceScale) 
            TEST_IGNORE()
            END_TEST_METHOD_ATTRIBUTE()
        TEST_CASE("CVMbyBforBalanceScale)
        {
            auto e = CVMultiByBinaryLearner("../datasets/balance-scale.int");
            CHECK(e < 0.9);
        }


        TEST_CASE("CVBoostingForIris)
        {
            auto e = CVBoostingLearner("../datasets/iris.int");
            CHECK(e < 0.1);
        }

        BEGIN_TEST_METHOD_ATTRIBUTE(CVBoostingForSoybean) 
            TEST_IGNORE()
        END_TEST_METHOD_ATTRIBUTE()
        TEST_CASE("CVBoostingForSoybean)
        {
            auto e = CVBoostingLearner("../datasets/soybean-large.int");
            CHECK(e < 0.17);
        }

        BEGIN_TEST_METHOD_ATTRIBUTE(CVBoostingForBalanceScale) 
            TEST_IGNORE()
            END_TEST_METHOD_ATTRIBUTE()
            TEST_CASE("CVBoostingForBalanceScale)
        {
            auto e = CVBoostingLearner("../datasets/balance-scale.int");
            CHECK(e < 0.5);
        }


    private:

        Estimate CheckMultiByBinaryLearner(std::string const& dataset)
        {
            BOOST_LOG_NAMED_SCOPE("CheckMultiByBinaryLearner");

            Dataset ds;
            ds.Load(dataset);

            MultiByBinaryLearner mbbc;  
            mbbc.CurrentOptions.correctPart = 1;

            auto ecv = std::make_shared<FTVotingLearner>();
            auto bfs = std::make_shared<InitECFeatureSelector>();
            ecv->SetFeatureSelector(bfs);
//            ecv->CurrentOptions.ftCount = 10;
            mbbc.SetPositiveBinaryLearner(ecv);

            ecv->GetOptions().coverLimit = 100;
            ecv->GetOptions().coverThreshold = 200;


            auto a = mbbc.Train(ds);

            GroupedPredictions gp;
            gp.groups.push_back(a->ApplyMany(ds.objects));
            auto e = gp.GetMeanError();
            LOG << "MeanError=" << e;
            return e;
        }

        Estimate CheckBoostingLearner(std::string const& dataset)
        {
            BOOST_LOG_NAMED_SCOPE("CheckBoostingLearner");

            Dataset ds;
            ds.Load(dataset);

            BoostingLearner boosting;    
            auto ecv = std::make_shared<FTVotingLearner>();
            auto bfs = std::make_shared<InitECFeatureSelector>();
            ecv->SetFeatureSelector(bfs);
//            ecv->CurrentOptions.ftCount = 5;
            boosting.positiveLearner = ecv;

            auto a = boosting.Train(ds);

            GroupedPredictions gp;
            gp.groups.push_back(a->ApplyMany(ds.objects));
            auto e = gp.GetMeanError();
            LOG << "MeanError=" << e;
            return e;
        }

        Estimate CVMultiByBinaryLearner(std::string const& dataset)
        {
            BOOST_LOG_NAMED_SCOPE("CVMultiByBinaryLearner");

            Dataset ds;
            ds.Load(dataset);

            auto mbbc = std::make_shared<MultiByBinaryLearner>();    
            auto ecv = std::make_shared<FTVotingLearner>();
            auto bfs = std::make_shared<InitECFeatureSelector>();
            ecv->SetFeatureSelector(bfs);
            ecv->GetOptions().coverLimit = 100;
            ecv->GetOptions().coverThreshold = 200;

//            ecv->CurrentOptions.ftCount = 10;
            mbbc->SetPositiveBinaryLearner(ecv);

            GroupedPredictions gp = AlgorithmEstimator().FoldsCrossValidation(ds, mbbc);
            auto e = gp.GetMeanError();
            LOG << "MeanError=" << e;
            return e;
        }

        Estimate CVBoostingLearner(std::string const& dataset)
        {
            BOOST_LOG_NAMED_SCOPE("CVBootingLearner");

            Dataset ds;
            ds.Load(dataset);

            auto boosting = std::make_shared<BoostingLearner>();    
            auto ecv = std::make_shared<FTVotingLearner>();
            auto bfs = std::make_shared<InitECFeatureSelector>();
            ecv->SetFeatureSelector(bfs);

//            ecv->CurrentOptions.ftCount = 5;
            boosting->positiveLearner = ecv;

            GroupedPredictions gp = AlgorithmEstimator().FoldsCrossValidation(ds, boosting);
            auto e = gp.GetMeanError();
            LOG << "MeanError=" << e;
            return e;
        }
    };
}