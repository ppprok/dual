#include "AlgorithmEstimator.h"
#include "BoostingLearner.h"
#include "FTVotingLearner.h"
#include "InitECFeatureSelector.h"
#include "ECVotingLearner.h"


namespace LogicalAnalysisLibTest
{
	TEST_CLASS(AlgorithmEstimatorTests)
	{
	public:
		
		TEST_CASE("LeaveOneOutCrossValidationTest)
		{
			AlgorithmEstimator ae;
            
            Dataset ds;
            //ds.Load("../datasets/lenses.int");
            ds.Load("../datasets/soybean-small.int");
            
            auto boosting = std::make_shared<BoostingLearner>();
            auto ecv = std::make_shared<FTVotingLearner>();
            /*auto bfs = std::make_shared<BoostingFeatureSelector>();
            bfs->CurrentOptions.mode = BoostingFeatureSelectorOptions::InitOnly;
            ecv->SetFeatureSelector(bfs);*/
//            ecv->CurrentOptions.ftCount = 1;
            boosting->positiveLearner = ecv;
            boosting->negativeLearner = ecv;
                        
            auto gp = ae.LeaveOneOutCrossValidation(ds, boosting);
            auto e = gp.GetMeanError();
            LOG_SEV(info) << "gp.GetMeanError()=" << gp.GetMeanError();
		}

	};
}