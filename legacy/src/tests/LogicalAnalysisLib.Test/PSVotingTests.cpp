#include "stdafx.h"
#include "CppUnitTest.h"
#include "Dataset.h"
#include "PSVotingLearner.h"
#include "logging_utils.h"
#include "MultiByBinaryLearner.h"
#include "AlgorithmEstimator.h"
#include "BoostingLearner.h"
#include "FTFeatureSelector.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LogicalAnalysisLibTest
{		
    TEST_CLASS(PSVotingTests)
    {
    public:

        TEST_CASE("PSVotingCorrect)
        {
            Dataset ds;
            ds.Load("../datasets/dermatology.int");

            auto parts = ds.Split(0.3f, 0.7f);
            parts.first.CollectClasses();
            parts.second.CollectClasses();
                        
            auto& correct = parts.first.classes[1];
            auto& fitting = parts.second.classes[1];

            PSVotingLearner ecv;
            ecv.GetFeatureSelector()->Init(ds);
            auto cls = ecv.Train(correct, fitting);

            for (auto& p:correct.positive)
            {
                LOG_SEV(trace) << "p:correct.positive: " << cls->Apply(*p);
                CHECK(cls->Apply(*p) > 0);
            }

            for (auto& no:correct.negative)
            {
                //LOG_SEV(trace) << "no:correct.negative: " << cls->Apply(*no);
                CHECK(cls->Apply(*no) == 0);
            }

            for (auto& p:fitting.positive)
            {
                LOG_SEV(trace) << "p:fitting.positive: " << cls->Apply(*p);
            }

            for (auto& p:fitting.negative)
            {
                LOG_SEV(trace) << "p:fitting.negative: " << cls->Apply(*p);
            }

            

        }

        TEST_CASE("MbyBforIris)
        {
            auto e = CheckMultiByBinaryLearner("../datasets/iris.int");
            CHECK(e < 0.1);
        }

        TEST_CASE("MbyBforSoybean)
        {
            auto e = CheckMultiByBinaryLearner("../datasets/soybean-large.int");
            CHECK(e < 0.2);
        }

        TEST_CASE("MbyBforBalanceScale)
        {
            auto e = CheckMultiByBinaryLearner("../datasets/balance-scale.int");
            CHECK(e < 0.3);
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

        TEST_CASE("BoostingForBalanceScale)
        {
            auto e = CheckBoostingLearner("../datasets/balance-scale.int");
            CHECK(e == 0);
        }

        TEST_CASE("CVMbyBforIris)
        {
            auto e = CVMultiByBinaryLearner("../datasets/iris.int");
            CHECK(e < 0.1);
        }

        TEST_CASE("CVMbyBforSoybean)
        {
            auto e = CVMultiByBinaryLearner("../datasets/soybean-large.int");
            CHECK(e < 0.4);
        }

        /*TEST_CASE("CVMbyBforBalanceScale)
        {
            auto e = CVMultiByBinaryLearner("../datasets/balance-scale.int");
            CHECK(e < 1);
        }*/


        TEST_CASE("CVBoostingForIris)
        {
            auto e = CVBoostingLearner("../datasets/iris.int");
            CHECK(e < 0.05);
        }

        TEST_CASE("CVBoostingForSoybean)
        {
            auto e = CVBoostingLearner("../datasets/soybean-large.int");
            CHECK(e < 0.15);
        }

        TEST_CASE("CVBoostingForBalanceScale)
        {
            auto e = CVBoostingLearner("../datasets/balance-scale.int");
            CHECK(e < 0.25);
        }


    private:

        Estimate CheckMultiByBinaryLearner(std::string const& dataset)
        {
            BOOST_LOG_NAMED_SCOPE("CheckMultiByBinaryLearner");

            Dataset ds;
            ds.Load(dataset);

            MultiByBinaryLearner mbbc;   
            mbbc.CurrentOptions.correctPart = 1;
            auto ecv = std::make_shared<PSVotingLearner>();
            ecv->GetOptions().coverLimit = 100;
            ecv->GetOptions().coverThreshold = 200;
            mbbc.SetPositiveBinaryLearner(ecv);

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
            boosting.CurrentOptions.iterationsCount = 1000;
            boosting.CurrentOptions.stopQ = 0.1f;
            auto ecv = std::make_shared<PSVotingLearner>();
            ecv->SetFeatureSelector(std::make_shared<FTFeatureSelector>());
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
            mbbc->CurrentOptions.correctPart = 1;
            
            auto ecv = std::make_shared<PSVotingLearner>();
            ecv->GetOptions().coverLimit = 100;
            ecv->GetOptions().coverThreshold = 200;

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
            auto ecv = std::make_shared<PSVotingLearner>();
            ecv->SetFeatureSelector(std::make_shared<FTFeatureSelector>());
            boosting->positiveLearner = ecv;

            GroupedPredictions gp = AlgorithmEstimator().FoldsCrossValidation(ds, boosting);
            auto e = gp.GetMeanError();
            LOG << "MeanError=" << e;
            return e;
        }
    };
}