#include "stdafx.h"
#include "CppUnitTest.h"
#include "RUNC.h"
#include "WeightBoundsPruning.h"
#include "CoverCollector.h"
#include "IntegersFileWriter.h"
#include "IntegersFileReader.h"
#include "Pruning.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DualizationLibTest
{
	TEST_CLASS(PruningTests)
	{
	public:
		
        TEST_CASE("FindMinCoverTest)
        {
            ChainCallback<WeightBoundsPruningCallback, 
                CoverCollector> c;
            c.callback1.TargetToMin();


            Runc::GlobalState A(Runc::RUNCM);
            A.LoadMatrix("../ao.test/datasets/10_50_7187.hg");
            A.InnerCallback = &c.callback1;
            A.CoverCallback = &c;
            A.Dualize();

            CHECK(c.callback2.GetCovers().size()>=78);

            int n = 0;
            for (auto& cover: c.callback2.GetCovers())
            {
                if (cover.columns.size() == 2)
                    ++n;
            }

            CHECK(n==78);

            IntegersFileWriter w("../ao.test/datasets/10_50_7187_min.hg");        
            w.WriteIntegerMatrix(c.callback2.GetCovers());
        }


        TEST_CASE("FindWeightedMinCoverTest)
        {
            ChainCallback<WeightBoundsPruningCallback, 
                CoverCollector> c;
            c.callback1.TargetToMin();

            Weights weights;
            for (Weight w = 0; w < 50; ++w)
                weights.push_back(expf(-fabs(25 - w)));

            c.callback1.SetWeights(std::move(weights));

            Runc::GlobalState A(Runc::RUNCM);
            A.LoadMatrix("../ao.test/datasets/10_50_7187.hg");
            A.InnerCallback = &c.callback1;
            A.CoverCallback = &c;
            A.Dualize();

            CHECK(c.callback2.GetCovers().size()>0);


            IntegersFileWriter w("../ao.test/datasets/10_50_7187_wmin.hg");        
            w.WriteIntegerMatrix(c.callback2.GetCovers());
        }

        TEST_CASE("FindMaxCoverTest)
        {
            ChainCallback<WeightBoundsPruningCallback, 
                CoverCollector> c;

            c.callback1.TargetToMax();

            Runc::GlobalState A(Runc::RUNCM);
            A.LoadMatrix("../ao.test/datasets/10_50_7187.hg");
            A.InnerCallback = &c;
            A.CoverCallback = &c;
            A.Dualize();

            CHECK(c.callback2.GetCovers().size()>=419);

            int n = 0;
            for (auto& cover: c.callback2.GetCovers())
            {
                if (cover.columns.size() == 5)
                    ++n;
            }

            CHECK(n==419);

            IntegersFileWriter w("../ao.test/datasets/10_50_7187_max.hg");        
            w.WriteIntegerMatrix(c.callback2.GetCovers());
        }


        TEST_CASE("FindWeightedMaxCoverTest)
        {
            ChainCallback<WeightBoundsPruningCallback, 
                CoverCollector> c;
            c.callback1.TargetToMax();

            Weights weights;
            for (Weight w = 0; w < 50; ++w)
                weights.push_back(exp(-abs(25 - w)));

            c.callback1.SetWeights(std::move(weights));

            Runc::GlobalState A(Runc::RUNCM);
            A.LoadMatrix("../ao.test/datasets/10_50_7187.hg");
            A.InnerCallback = &c.callback1;
            A.CoverCallback = &c;
            A.Dualize();

            CHECK(c.callback2.GetCovers().size()>0);

            IntegersFileWriter w("../ao.test/datasets/10_50_7187_wmax.hg");        
            w.WriteIntegerMatrix(c.callback2.GetCovers());
        }

/*        TEST_CASE("CountCoveredRowsTest)
        {
            ChainCallback<CoveredRowsWeighter, 
                CoverCollector> c;

            auto mat = IntegersFileReader("../ao.test/datasets/6_6_4_depRows.hg").ReadBitMatrix();

            CoveredRowsWeighter::Options options;
            options.TargetRows = mat.get_rows();
            options.DefaultWeight = 1;

            c.callback1.SetOptions(std::move(options));

            Runc::GlobalState A(Runc::RUNCM);
            A.LoadMatrix("../ao.test/datasets/6_6_4.hg");
            A.InnerCallback = &c.callback1;
            A.CoverCallback = &c;
            A.Dualize();

            CHECK(c.callback2.GetCovers().size()==4);
            CHECK(c.callback2.GetCovers()[0].weight == 7);
            CHECK(c.callback2.GetCovers()[1].weight == 7);
            CHECK(c.callback2.GetCovers()[2].weight == 7);
            CHECK(c.callback2.GetCovers()[3].weight == 6);

        }*/

/*        TEST_CASE("WeightCoveredRowsTest)
        {
            ChainCallback<CoveredRowsWeighter, 
                CoverCollector> c;

            auto targetMatrix = IntegersFileReader("../ao.test/datasets/6_6_4_depRows.hg").ReadBitMatrix();
            for (auto r : targetMatrix.get_rows())
                r.flip();

            CoveredRowsWeighter::Options options;
            options.RowsWeights = list_of(1.0f)(2.0f)(3.0f)(4.0f)(5.0f)(6.0f)(7.0f);
            options.TargetRows = targetMatrix.get_rows();

            c.callback1.SetOptions(std::move(options));

            Runc::GlobalState A(Runc::RUNCM);
            A.LoadMatrix("../ao.test/datasets/6_6_4.hg");
            A.InnerCallback = &c.callback1;
            A.CoverCallback = &c;
            A.Dualize();

            CHECK(c.callback2.GetCovers().size()==4);
            CHECK(c.callback2.GetCovers()[0].weight == 28-14);
            CHECK(c.callback2.GetCovers()[1].weight == 28-15);
            CHECK(c.callback2.GetCovers()[2].weight == 28-22);
            CHECK(c.callback2.GetCovers()[3].weight == 28-13);

        }*/

	};
}