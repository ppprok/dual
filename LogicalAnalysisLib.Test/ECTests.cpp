#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\LogicalAnalisysLib\EC.h"
#include "..\LogicalAnalisysLib\ECmp.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LogicalAnalysisLibTest
{
	TEST_CLASS(ECTests)
	{
	public:
				
        // Проверка эл.кл.
        TEST_METHOD( ECTest )  
        {
            Object o;
            o.values = list_of(1)(2)(0)(4);

            EC ec1;
            ec1.featureValues.emplace_back(1,2);
            ec1.featureValues.emplace_back(3,4);

            Assert::IsTrue(ec1.Apply(o));

            EC ec2;
            ec2.featureValues.emplace_back(1,2);
            ec2.featureValues.emplace_back(0,0);

            Assert::IsTrue(! ec2.Apply(o));
        }

        // Проверка эл.кл. с отношением меньше или равно
        TEST_METHOD( ECmpLETest )  
        {
            Object o1;
            o1.values = list_of(1)(2)(0)(4);

            Object o2;
            o2.values = list_of(2)(2)(0)(1);

            EC ec1;
            ec1.featureValues.emplace_back(1,2);
            ec1.featureValues.emplace_back(2,0);

            Assert::IsTrue(ec1.Apply(o1));
            Assert::IsTrue(ec1.Apply(o2));
            Assert::IsTrue(ECmp(ec1, ECmp::LE).Apply(o1,o2));
            Assert::IsTrue(ECmp(ec1, ECmp::LE).Apply(o2,o1));


            EC ec2;
            ec2.featureValues.emplace_back(0,1);
            ec2.featureValues.emplace_back(2,0);

            Assert::IsTrue(ec2.Apply(o1));
            Assert::IsTrue(! ec2.Apply(o2));
            Assert::IsTrue(! ECmp(ec2, ECmp::LE).Apply(o1,o2));
            Assert::IsTrue(ECmp(ec2, ECmp::LE).Apply(o2,o1));

            EC ec3;
            ec3.featureValues.emplace_back(1,2);
            ec3.featureValues.emplace_back(3,0);

            Assert::IsTrue(! ec3.Apply(o1));
            Assert::IsTrue(! ec3.Apply(o2));
            Assert::IsTrue(ECmp(ec3, ECmp::LE).Apply(o1,o2));
            Assert::IsTrue(ECmp(ec3, ECmp::LE).Apply(o2,o1));

        }

        // Проверка эл.кл. с отношением больше или равно
        TEST_METHOD( ECmpGETest )  
        {
            Object o1;
            o1.values = list_of(1)(2)(0)(4);

            Object o2;
            o2.values = list_of(2)(2)(0)(1);

            EC ec1;
            ec1.featureValues.emplace_back(1,2);
            ec1.featureValues.emplace_back(2,0);

            ECmp::RelationType op = ECmp::GE;

            Assert::IsTrue(ec1.Apply(o1));
            Assert::IsTrue(ec1.Apply(o2));
            Assert::IsTrue(ECmp(ec1, op).Apply(o1,o2));
            Assert::IsTrue(ECmp(ec1, op).Apply(o2,o1));


            EC ec2;
            ec2.featureValues.emplace_back(0,1);
            ec2.featureValues.emplace_back(2,0);

            Assert::IsTrue(ec2.Apply(o1));
            Assert::IsTrue(! ec2.Apply(o2));
            Assert::IsTrue(ECmp(ec2, op).Apply(o1,o2));
            Assert::IsTrue(! ECmp(ec2, op).Apply(o2,o1));

            EC ec3;
            ec3.featureValues.emplace_back(1,2);
            ec3.featureValues.emplace_back(3,0);

            Assert::IsTrue(! ec3.Apply(o1));
            Assert::IsTrue(! ec3.Apply(o2));
            Assert::IsTrue(ECmp(ec3, op).Apply(o1,o2));
            Assert::IsTrue(ECmp(ec3, op).Apply(o2,o1));

        }


        // Проверка эл.кл. с отношением ИЛИ
        TEST_METHOD( ECmpOrTest )  
        {
            Object o1;
            o1.values = list_of(1)(2)(0)(4);

            Object o2;
            o2.values = list_of(2)(2)(0)(1);

            EC ec1;
            ec1.featureValues.emplace_back(1,2);
            ec1.featureValues.emplace_back(2,0);

            ECmp::RelationType op = ECmp::Or;

            Assert::IsTrue(ec1.Apply(o1));
            Assert::IsTrue(ec1.Apply(o2));
            Assert::IsTrue(ECmp(ec1, op).Apply(o1,o2));
            Assert::IsTrue(ECmp(ec1, op).Apply(o2,o1));

            EC ec2;
            ec2.featureValues.emplace_back(0,1);
            ec2.featureValues.emplace_back(2,0);

            Assert::IsTrue(ec2.Apply(o1));
            Assert::IsTrue(! ec2.Apply(o2));
            Assert::IsTrue(ECmp(ec2, op).Apply(o1,o2));
            Assert::IsTrue(ECmp(ec2, op).Apply(o2,o1));

            EC ec3;
            ec3.featureValues.emplace_back(1,2);
            ec3.featureValues.emplace_back(3,0);

            Assert::IsTrue(! ec3.Apply(o1));
            Assert::IsTrue(! ec3.Apply(o2));
            Assert::IsTrue(! ECmp(ec3, op).Apply(o1,o2));
            Assert::IsTrue(! ECmp(ec3, op).Apply(o2,o1));

        }


        // Проверка эл.кл. с отношением НЕ И
        TEST_METHOD( ECmpNotAndTest )  
        {
            Object o1;
            o1.values = list_of(1)(2)(0)(4);

            Object o2;
            o2.values = list_of(2)(2)(0)(1);

            EC ec1;
            ec1.featureValues.emplace_back(1,2);
            ec1.featureValues.emplace_back(2,0);

            ECmp::RelationType op = ECmp::NotAnd;

            Assert::IsTrue(ec1.Apply(o1));
            Assert::IsTrue(ec1.Apply(o2));
            Assert::IsTrue(! ECmp(ec1, op).Apply(o1,o2));
            Assert::IsTrue(! ECmp(ec1, op).Apply(o2,o1));

            EC ec2;
            ec2.featureValues.emplace_back(0,1);
            ec2.featureValues.emplace_back(2,0);

            Assert::IsTrue(ec2.Apply(o1));
            Assert::IsTrue(! ec2.Apply(o2));
            Assert::IsTrue(ECmp(ec2, op).Apply(o1,o2));
            Assert::IsTrue(ECmp(ec2, op).Apply(o2,o1));

            EC ec3;
            ec3.featureValues.emplace_back(1,2);
            ec3.featureValues.emplace_back(3,0);

            Assert::IsTrue(! ec3.Apply(o1));
            Assert::IsTrue(! ec3.Apply(o2));
            Assert::IsTrue(ECmp(ec3, op).Apply(o1,o2));
            Assert::IsTrue(ECmp(ec3, op).Apply(o2,o1));

        }

	};
}