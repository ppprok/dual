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
				
        // �������� ��.��.
        TEST_CASE(" ECTest )
        {
            Object o;
            o.values = list_of(1)(2)(0)(4);

            EC ec1;
            ec1.featureValues.emplace_back(1,2);
            ec1.featureValues.emplace_back(3,4);

            CHECK(ec1.Apply(o));

            EC ec2;
            ec2.featureValues.emplace_back(1,2);
            ec2.featureValues.emplace_back(0,0);

            CHECK(! ec2.Apply(o));
        }

        // �������� ��.��. � ���������� ������ ��� �����
        TEST_CASE(" ECmpLETest )
        {
            Object o1;
            o1.values = list_of(1)(2)(0)(4);

            Object o2;
            o2.values = list_of(2)(2)(0)(1);

            EC ec1;
            ec1.featureValues.emplace_back(1,2);
            ec1.featureValues.emplace_back(2,0);

            CHECK(ec1.Apply(o1));
            CHECK(ec1.Apply(o2));
            CHECK(ECmp(ec1, ECmp::LE).Apply(o1,o2));
            CHECK(ECmp(ec1, ECmp::LE).Apply(o2,o1));


            EC ec2;
            ec2.featureValues.emplace_back(0,1);
            ec2.featureValues.emplace_back(2,0);

            CHECK(ec2.Apply(o1));
            CHECK(! ec2.Apply(o2));
            CHECK(! ECmp(ec2, ECmp::LE).Apply(o1,o2));
            CHECK(ECmp(ec2, ECmp::LE).Apply(o2,o1));

            EC ec3;
            ec3.featureValues.emplace_back(1,2);
            ec3.featureValues.emplace_back(3,0);

            CHECK(! ec3.Apply(o1));
            CHECK(! ec3.Apply(o2));
            CHECK(ECmp(ec3, ECmp::LE).Apply(o1,o2));
            CHECK(ECmp(ec3, ECmp::LE).Apply(o2,o1));

        }

        // �������� ��.��. � ���������� ������ ��� �����
        TEST_CASE(" ECmpGETest )
        {
            Object o1;
            o1.values = list_of(1)(2)(0)(4);

            Object o2;
            o2.values = list_of(2)(2)(0)(1);

            EC ec1;
            ec1.featureValues.emplace_back(1,2);
            ec1.featureValues.emplace_back(2,0);

            ECmp::RelationType op = ECmp::GE;

            CHECK(ec1.Apply(o1));
            CHECK(ec1.Apply(o2));
            CHECK(ECmp(ec1, op).Apply(o1,o2));
            CHECK(ECmp(ec1, op).Apply(o2,o1));


            EC ec2;
            ec2.featureValues.emplace_back(0,1);
            ec2.featureValues.emplace_back(2,0);

            CHECK(ec2.Apply(o1));
            CHECK(! ec2.Apply(o2));
            CHECK(ECmp(ec2, op).Apply(o1,o2));
            CHECK(! ECmp(ec2, op).Apply(o2,o1));

            EC ec3;
            ec3.featureValues.emplace_back(1,2);
            ec3.featureValues.emplace_back(3,0);

            CHECK(! ec3.Apply(o1));
            CHECK(! ec3.Apply(o2));
            CHECK(ECmp(ec3, op).Apply(o1,o2));
            CHECK(ECmp(ec3, op).Apply(o2,o1));

        }


        // �������� ��.��. � ���������� ���
        TEST_CASE(" ECmpOrTest )
        {
            Object o1;
            o1.values = list_of(1)(2)(0)(4);

            Object o2;
            o2.values = list_of(2)(2)(0)(1);

            EC ec1;
            ec1.featureValues.emplace_back(1,2);
            ec1.featureValues.emplace_back(2,0);

            ECmp::RelationType op = ECmp::Or;

            CHECK(ec1.Apply(o1));
            CHECK(ec1.Apply(o2));
            CHECK(ECmp(ec1, op).Apply(o1,o2));
            CHECK(ECmp(ec1, op).Apply(o2,o1));

            EC ec2;
            ec2.featureValues.emplace_back(0,1);
            ec2.featureValues.emplace_back(2,0);

            CHECK(ec2.Apply(o1));
            CHECK(! ec2.Apply(o2));
            CHECK(ECmp(ec2, op).Apply(o1,o2));
            CHECK(ECmp(ec2, op).Apply(o2,o1));

            EC ec3;
            ec3.featureValues.emplace_back(1,2);
            ec3.featureValues.emplace_back(3,0);

            CHECK(! ec3.Apply(o1));
            CHECK(! ec3.Apply(o2));
            CHECK(! ECmp(ec3, op).Apply(o1,o2));
            CHECK(! ECmp(ec3, op).Apply(o2,o1));

        }


        // �������� ��.��. � ���������� �� �
        TEST_CASE(" ECmpNotAndTest )
        {
            Object o1;
            o1.values = list_of(1)(2)(0)(4);

            Object o2;
            o2.values = list_of(2)(2)(0)(1);

            EC ec1;
            ec1.featureValues.emplace_back(1,2);
            ec1.featureValues.emplace_back(2,0);

            ECmp::RelationType op = ECmp::NotAnd;

            CHECK(ec1.Apply(o1));
            CHECK(ec1.Apply(o2));
            CHECK(! ECmp(ec1, op).Apply(o1,o2));
            CHECK(! ECmp(ec1, op).Apply(o2,o1));

            EC ec2;
            ec2.featureValues.emplace_back(0,1);
            ec2.featureValues.emplace_back(2,0);

            CHECK(ec2.Apply(o1));
            CHECK(! ec2.Apply(o2));
            CHECK(ECmp(ec2, op).Apply(o1,o2));
            CHECK(ECmp(ec2, op).Apply(o2,o1));

            EC ec3;
            ec3.featureValues.emplace_back(1,2);
            ec3.featureValues.emplace_back(3,0);

            CHECK(! ec3.Apply(o1));
            CHECK(! ec3.Apply(o2));
            CHECK(ECmp(ec3, op).Apply(o1,o2));
            CHECK(ECmp(ec3, op).Apply(o2,o1));

        }

	};
}