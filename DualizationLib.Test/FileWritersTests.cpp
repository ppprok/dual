#include "stdafx.h"
#include "CppUnitTest.h"
#include "FileWriters.h"
#include "RUNC.h"
#include "IntegersFileReader.h"
#include "WeightBoundsPruning.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DualizationLibTest
{
	TEST_CLASS(FileWritersTests)
	{
	public:
		
        TEST_METHOD(AllWriterTest)
        {
            AllColumnsFileWriterCallback w;
            w.CreateOutputFile("../ao.test/datasets/6_6_4_dual.hg");        

            Runc::GlobalState A(Runc::RUNCM);
            A.LoadMatrix("../ao.test/datasets/6_6_4.hg");
            A.InnerCallback = &w;
            A.CoverCallback = &w;
            A.Dualize();

            w.CloseOutputFile();

            IntegersFileReader r("../ao.test/datasets/6_6_4_dual.hg");

            std::vector<std::vector<int>> hg;
            r.ReadUnsignedIntegersMatrix(hg);

            Assert::IsTrue(hg.size()==4);

            std::vector<std::vector<int>> etalon;
            etalon.push_back(list_of(1)(4));
            etalon.push_back(list_of(1)(5)(0));
            etalon.push_back(list_of(1)(5)(3));
            etalon.push_back(list_of(4)(3));

            Assert::IsTrue(hg == etalon);
        }

        TEST_METHOD(OrderedWriterTest)
        {
            OrderedColumnsFileWriterCallback w;
            w.CreateOutputFile("../ao.test/datasets/6_6_4_ordered.hg");        

            Runc::GlobalState A(Runc::RUNCM);
            A.LoadMatrix("../ao.test/datasets/6_6_4.hg");
            A.InnerCallback = &w;
            A.CoverCallback = &w;

            A.Dualize();

            w.CloseOutputFile();

            IntegersFileReader r("../ao.test/datasets/6_6_4_ordered.hg");

            std::vector<std::vector<int>> hg;
            r.ReadUnsignedIntegersMatrix(hg);

            Assert::IsTrue(hg.size()==4);

            std::vector<std::vector<int>> etalon;
            etalon.push_back(list_of(1)(4));
            etalon.push_back(list_of(0)(1)(5));
            etalon.push_back(list_of(1)(3)(5));
            etalon.push_back(list_of(3)(4));

            Assert::IsTrue(hg == etalon);
        }

        TEST_METHOD(SizeWriterTest)
        {
            SizeFileWriterCallback w;
            w.CreateOutputFile("../ao.test/datasets/6_6_4_size.hg");        

            Runc::GlobalState A(Runc::RUNCM);
            A.LoadMatrix("../ao.test/datasets/6_6_4.hg");
            A.InnerCallback = &w;
            A.CoverCallback = &w;

            A.Dualize();

            w.CloseOutputFile();

            IntegersFileReader r("../ao.test/datasets/6_6_4_size.hg");

            std::vector<std::vector<int>> hg;
            r.ReadUnsignedIntegersMatrix(hg);

            Assert::IsTrue(hg.size()==4);
            
            sort(hg);

            std::vector<std::vector<int>> etalon;
            etalon.push_back(list_of(2));
            etalon.push_back(list_of(2));
            etalon.push_back(list_of(3));
            etalon.push_back(list_of(3));

            Assert::IsTrue(hg == etalon);
        }

        TEST_METHOD(WeightWriterTest)
        {
            ChainCallback<WeightBoundsPruningCallback, 
                WeightFileWriterCallback> c;
            c.callback1.TargetToMax();
            c.callback2.CreateOutputFile("../ao.test/datasets/10_50_7187_dual_max.weights");

            Weights weights;
            for (Weight w = 0; w < 50; ++w)
                weights.push_back(exp(-abs(25 - w)));

            c.callback1.SetWeights(std::move(weights));

            Runc::GlobalState A(Runc::RUNCM);
            A.LoadMatrix("../ao.test/datasets/10_50_7187.hg");
            A.InnerCallback = &c.callback1;
            A.CoverCallback = &c;
            A.Dualize();
        }


        TEST_METHOD(DualDualWriterTest)
        {
            OrderedColumnsFileWriterCallback w;
            w.CreateOutputFile("../ao.test/datasets/10_50_7187_dual.hg");        

            Runc::GlobalState A(Runc::RUNCM);
            A.LoadMatrix("../ao.test/datasets/10_50_7187.hg");
            A.InnerCallback = &w;
            A.CoverCallback = &w;

            A.Dualize();

            w.CloseOutputFile();

            std::vector<std::vector<int>> hg;
            IntegersFileReader("../ao.test/datasets/10_50_7187_dual.hg").ReadUnsignedIntegersMatrix(hg);
            Assert::IsTrue(hg.size()==7187);

            w.CreateOutputFile("../ao.test/datasets/10_50_7187_dual_dual.hg");
            A.LoadMatrix("../ao.test/datasets/10_50_7187_dual.hg");
            A.Dualize();
            w.CloseOutputFile();

            hg.clear();
            IntegersFileReader("../ao.test/datasets/10_50_7187_dual_dual.hg").ReadUnsignedIntegersMatrix(hg);
            Assert::IsTrue(hg.size()==10);

        }
	};
}