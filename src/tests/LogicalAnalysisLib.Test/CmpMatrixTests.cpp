#include "CmpMatrix.h"
#include "Dataset.h"
#include "FTFeatureSelector.h"
#include "CmpFitMatrixBuilder.h"

    TEST_CLASS(CmpMatrixTests)
    {
    public:

        TEST_CASE("LoadFittingObjectsTest)
        {
            Dataset ds;
            ds.Load("../datasets/soybean-small.int");
                        
            auto parts = ds.Split(0.5f, 0.5f);

            parts.first.CollectClasses();
            parts.second.CollectClasses();

            auto& correct = parts.first.classes[0];
            auto& fitting = parts.second.classes[0];

            for(int i = 0, m = fitting.positive.size(); i < m; ++i)
                fitting.positive[i]->weight = 0.01f * (rand() % 100);

            for(int i = 0, m = fitting.negative.size(); i < m; ++i)
                fitting.negative[i]->weight = 0.01f * (rand() % 100);

            CmpFitMatrixBuilder mb;

            auto fc = std::make_shared<FeatureComparer>();
            FTFeatureSelector fs;
            fs.Init(ds);
            fc->features = fs.GetFeatures();

            mb.SetComparer(fc);
            
            mb.Build(correct, fitting);

            mb.FindCovers();

            std::vector<Features> tests;

            for (auto& r : mb.GetCoverMatrix().GetCovers().get_rows())
            {
                Features f;
                fc->Get(r, f);
                tests.emplace_back(std::move(f));
            }
        }

    };
