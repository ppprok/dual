#include "stdafx.h"
#include "CppUnitTest.h"
#include "PosetSelfTester.h"
#include "NumberChain.h"
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DualizationLibTest
{
	
    
    TEST_CLASS(NumberChainPosetTests)
	{
	public:

        typedef int Number;
		
		PosetSelfTester tester;

		NumberChainPosetTests()
			:tester(std::make_shared<NumberChain>())
		{
		}

		TEST_CASE("SelectFilterTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4);
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);
		
			PosetItems selected = tester.SelectFilterSelfTest(A, D);
			PosetItems expected = list_of<Number>(-2)(-3)(3)(4);

			CHECK(tester.Equals(expected, selected));
		}

		TEST_CASE("SelectFilterEmptyTest)
		{
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);
			PosetItems selected = tester.SelectFilterSelfTest(PosetItems(), D);			
			CHECK(selected.empty());
		}

		TEST_CASE("SelectIdealTest)
		{
			PosetItems A = list_of<Number>(1)(-1)(3)(0)(4);
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);

			PosetItems selected = tester.SelectIdealSelfTest(A, D);
			PosetItems expected = list_of<Number>(5)(3)(4)(6);

			CHECK(tester.Equals(expected, selected));
		}

		TEST_CASE("SelectIdealEmptyTest)
		{
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);
			PosetItems selected = tester.SelectIdealSelfTest(PosetItems(), D);			
			CHECK(selected.empty());
		}

		TEST_CASE("SelectFilterIndependentTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4);
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);

			PosetItems selected = tester.SelectFilterIndependentSelfTest(A, D);
			PosetItems expected = list_of<Number>(5)(6);

			CHECK(tester.Equals(expected, selected));
		}

		TEST_CASE("SelectFilterIndependentEmptyTest)
		{
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);
			PosetItems selected = tester.SelectFilterIndependentSelfTest(PosetItems(), D);			
			CHECK(tester.Equals(selected, D));
		}

		TEST_CASE("SelectIdealIndependentTest)
		{
			PosetItems A = list_of<Number>(1)(-1)(3)(0)(4);
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);

			PosetItems selected = tester.SelectIdealIndependentSelfTest(A, D);
			PosetItems expected = list_of<Number>(-2)(-3);

			CHECK(tester.Equals(expected, selected));
		}

		TEST_CASE("SelectIdealIndependentEmptyTest)
		{
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);
			PosetItems selected = tester.SelectIdealIndependentSelfTest(PosetItems(), D);			
			CHECK(tester.Equals(selected, D));
		}

		TEST_CASE("SelectMaximalTest)
		{
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);

			PosetItems selected = tester.SelectMaximalSelfTest(D);
			PosetItems expected = list_of<Number>(6);

			CHECK(tester.Equals(expected, selected));
		}

		TEST_CASE("SelectMinimalTest)
		{
			PosetItems D = list_of<Number>(-3)(5)(-3)(3)(4)(6);

			PosetItems selected = tester.SelectMinimalSelfTest(D);
			PosetItems expected = list_of<Number>(-3);

			CHECK(tester.Equals(expected, selected));
		}

		TEST_CASE("SelectUniqueTest)
		{
			PosetItems D = list_of<Number>(-3)(5)(-3)(3)(3)(5);

			PosetItems selected = tester.SelectUniqueSelfTest(D);
			PosetItems expected = list_of<Number>(-3)(3)(5);

			CHECK(tester.Equals(expected, selected));
		}

		TEST_CASE("PreceqTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4)(0);			
			tester.PreceqSelfTest(A);
		}

		TEST_CASE("PrecTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4)(0);			
			tester.PrecSelfTest(A);
		}

		TEST_CASE("EqualTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4)(0)(3)(-4);			
			tester.EqualSelfTest(A);
		}

		TEST_CASE("IsMaximalFromTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4)(0)(3)(-4);			
			int r = tester.IsMaximalFromSelfTest(A);
			CHECK_EQ(1, r);
		}

		TEST_CASE("IsMinimalFromTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4)(0)(3)(-4);			
			int r = tester.IsMinimalFromSelfTest(A);
			CHECK_EQ(2, r);
		}

		TEST_CASE("IsItemTest)
		{
			PosetItems A = list_of<Number>(1)(3)(-2)(4)(0)(-4);
			A.push_back(tester.GetPoset()->GetUniversalMaximal());
			A.push_back(tester.GetPoset()->GetUniversalMinimal());
			tester.IsItemSelfTest(A, PosetItems());
			
		}

		TEST_CASE("ToStringTest)
		{
			PosetItems A = list_of<Number>(1)(3)(-2)(4)(0)(-4);
			A.push_back(tester.GetPoset()->GetUniversalMaximal());
			A.push_back(tester.GetPoset()->GetUniversalMinimal());
			
			tester.ToStringConvertSelfTest(A);
		}

		TEST_CASE("FromStringTest)
		{
			Strings A = list_of("1")("3")("-2")("4")("0")("-4")("+\\infty")("-\\infty");
			tester.FromStringConvertSelfTest(A);
		}

        TEST_CASE("GetMinimalTest)
        {
            PosetItems A = list_of<Number>(1)(3)(-2)(4)(0)(-4);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            auto minimal = tester.GetMinimalSelfTest(A);
            PosetItems expected(1, tester.GetPoset()->GetUniversalMinimal());
            CHECK(tester.Equals(expected, minimal));
        }

        TEST_CASE("GetMaximalTest)
        {
            PosetItems A = list_of<Number>(1)(3)(-2)(4)(0)(-4);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            auto maximal = tester.GetMaximalSelfTest(A);
            PosetItems expected(1, tester.GetPoset()->GetUniversalMaximal());
            CHECK(tester.Equals(expected, maximal));
        }

        TEST_CASE("TopologicalSortTest)
        {
            PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4)(0)(3)(-4);			
            auto B = tester.TopologicalSortSelfTest(A);
            PosetItems C = list_of<Number>(4)(3)(3)(1)(0)(-2)(-4)(-4);
            CHECK(tester.Equals(B, C));
        }


        TEST_CASE("PrecImmediateTest)
        {
            PosetItems A = list_of(1)(2)(3)(4)(5)(6)(7);
            //A.push_back(tester.GetPoset()->GetUniversalMinimal());
            //A.push_back(tester.GetPoset()->GetUniversalMaximal());

            tester.PrecImmediateSelfTest(A);
        }

        TEST_CASE("GetImmediatePrecTest)
        {
            PosetItems A = list_of(1)(2)(3)(6)(7);
            A.push_back(tester.GetPoset()->GetUniversalMinimal());
            A.push_back(tester.GetPoset()->GetUniversalMaximal());

            tester.GetImmediatePrecSelfTest(A);
        }


        TEST_CASE("GetImmediateSuccTest)
        {
            PosetItems A = list_of(1)(2)(3)(6)(7);
            A.push_back(tester.GetPoset()->GetUniversalMinimal());
            A.push_back(tester.GetPoset()->GetUniversalMaximal());

            tester.GetImmediateSuccSelfTest(A);
        }

        TEST_CASE("IsMinimalIndependentTest)
        {
            PosetItems candidates=list_of(-1)(-2)(0)(1)(2)(6)(7)(5);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(3)(5);

            tester.IsMinimalIndependentSelfTest(A, candidates);
        }


        TEST_CASE("GetMinimalIndependentTest)
        {
            PosetItems candidates=list_of(-1)(-2)(0)(1)(2)(6)(7)(5);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(3)(5);
            auto indep =  tester.GetMinimalIndependentSelfTest(A, candidates);

            PosetItems expected = list_of(6);
            CHECK(tester.Equals(expected, indep));
        }

        TEST_CASE("IsMaximalIndependentTest)
        {
            PosetItems candidates=list_of(-1)(-2)(0)(1)(2);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(2)(1);

            tester.IsMaximalIndependentSelfTest(A, candidates);
        }


        TEST_CASE("GetMaximalIndependentTest)
        {
            PosetItems candidates=list_of(-1)(-2)(0)(1)(2)(3);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(2)(1);

            auto indep = tester.GetMaximalIndependentSelfTest(A, candidates);

            PosetItems expected = list_of(0);
            CHECK(tester.Equals(expected, indep));
        }

        TEST_CASE("GetIndependentIntervalsTest)
        {
            PosetItems candidates = list_of(1)(2)(3)(4)(5)(6)(7);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(4)(6);
            PosetItems B = list_of(1)(0)(2);

            auto indep = tester.GetIndependentIntervalsSelfTest(A, B, candidates);

            CHECK_EQ(1, indep);
        }

	};
}