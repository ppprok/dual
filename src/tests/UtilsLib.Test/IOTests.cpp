#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\UtilsLib\IntegersFileWriter.h"
#include "..\UtilsLib\IntegersFileReader.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UtilsLibTest
{
	TEST_CLASS(IOTests)
	{
	public:
		
		
        TEST_METHOD(write_interger_test)
        {
            IntegersFileWriter writer("../ao.test/datasets/integers.txt");	        
            std::vector<int> integers = list_of(100)(100000500)(190051001);        
            writer.WriteIntegers(integers);        
            writer.CloseOutputFile();

            IntegersFileReader reader("../ao.test/datasets/integers.txt");
            std::vector<int> result;
            reader.ReadUnsignedIntegers(result);
            Assert::IsTrue(integers == result);
        }


        TEST_METHOD(read_hypergraph_test)
        {
            IntegersFileReader reader("../ao.test/datasets/6_6_6.hg");

            std::vector<std::vector<int>> hg;

            reader.ReadUnsignedIntegersMatrix(hg);

            Assert::IsTrue(hg.size() == 6);

            std::vector<std::vector<int>> etalon;
            etalon.push_back(list_of(0)(1)(3)(4)(5));
            etalon.push_back(list_of(0)(1)(2)(3)(4)(5));
            etalon.push_back(list_of(1)(3));
            etalon.push_back(list_of(3)(4));
            etalon.push_back(list_of(0)(1)(2)(4)(5));
            etalon.push_back(list_of(2)(3));

            Assert::IsTrue(hg == etalon);

        }

        TEST_METHOD(read_iris_test)
        {
            IntegersFileReader reader("../ao.test/datasets/iris.txt");

            std::vector<std::vector<int>> hg;

            reader.ReadUnsignedIntegersMatrix(hg);

            Assert::IsTrue(hg.size() == 77);
            hg.pop_back();

            for (auto v:hg)
            {
                Assert::IsTrue(v.size() == 5);
            }

        }

        TEST_METHOD(read_bit_matrix)
        {
            IntegersFileReader reader("../ao.test/datasets/6_6_6.hg");

            bit_matrix bm = reader.ReadBitMatrix();

            Assert::IsTrue(bm.height() == 6);
            Assert::IsTrue(bm.width() == 6);

            std::vector<std::vector<int>> etalon;
            etalon.push_back(list_of(0)(1)(3)(4)(5));
            etalon.push_back(list_of(0)(1)(2)(3)(4)(5));
            etalon.push_back(list_of(1)(3));
            etalon.push_back(list_of(3)(4));
            etalon.push_back(list_of(0)(1)(2)(4)(5));
            etalon.push_back(list_of(2)(3));

            for(int i = 0; i < 6; ++i)
            {
                for (int j : etalon[i])
                {
                    Assert::IsTrue(bm.get(i,j));
                }
                Assert::IsTrue(bm.get_row(i).count()==etalon[i].size());
            }

        }

        TEST_METHOD(write_bit_matrix)
        {
            bit_matrix bm;

            bm.set(0,1);
            bm.set(0,10);
            bm.set(0,100);

            bm.set(1,11);
            bm.set(1,7);
            bm.set(1,10);

            bm.set(2,110);
            bm.set(2,76);
            bm.set(2,50);
            bm.set(2,5);

            bm.set(3,15);
            bm.set(3,95);

            Assert::IsTrue(bm.height()==4);
            Assert::IsTrue(bm.width()==111);

            IntegersFileWriter writer("../ao.test/datasets/write_bm_test.hg");
            writer.WriteBitMatrix(bm);
            writer.CloseOutputFile();

            IntegersFileReader reader("../ao.test/datasets/write_bm_test.hg");

            std::vector<std::vector<int>> hg;
            reader.ReadUnsignedIntegersMatrix(hg);

            std::vector<std::vector<int>> etalon;
            etalon.push_back(list_of(1)(10)(100));
            etalon.push_back(list_of(7)(10)(11));
            etalon.push_back(list_of(5)(50)(76)(110));
            etalon.push_back(list_of(15)(95));

            Assert::IsTrue(hg == etalon);

        }

	};
}