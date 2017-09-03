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

		TEST_METHOD(SelectFilterTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4);
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);
		
			PosetItems selected = tester.SelectFilterSelfTest(A, D);
			PosetItems expected = list_of<Number>(-2)(-3)(3)(4);

			Assert::IsTrue(tester.Equals(expected, selected));
		}

		TEST_METHOD(SelectFilterEmptyTest)
		{
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);
			PosetItems selected = tester.SelectFilterSelfTest(PosetItems(), D);			
			Assert::IsTrue(selected.empty());
		}

		TEST_METHOD(SelectIdealTest)
		{
			PosetItems A = list_of<Number>(1)(-1)(3)(0)(4);
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);

			PosetItems selected = tester.SelectIdealSelfTest(A, D);
			PosetItems expected = list_of<Number>(5)(3)(4)(6);

			Assert::IsTrue(tester.Equals(expected, selected));
		}

		TEST_METHOD(SelectIdealEmptyTest)
		{
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);
			PosetItems selected = tester.SelectIdealSelfTest(PosetItems(), D);			
			Assert::IsTrue(selected.empty());
		}

		TEST_METHOD(SelectFilterIndependentTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4);
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);

			PosetItems selected = tester.SelectFilterIndependentSelfTest(A, D);
			PosetItems expected = list_of<Number>(5)(6);

			Assert::IsTrue(tester.Equals(expected, selected));
		}

		TEST_METHOD(SelectFilterIndependentEmptyTest)
		{
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);
			PosetItems selected = tester.SelectFilterIndependentSelfTest(PosetItems(), D);			
			Assert::IsTrue(tester.Equals(selected, D));
		}

		TEST_METHOD(SelectIdealIndependentTest)
		{
			PosetItems A = list_of<Number>(1)(-1)(3)(0)(4);
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);

			PosetItems selected = tester.SelectIdealIndependentSelfTest(A, D);
			PosetItems expected = list_of<Number>(-2)(-3);

			Assert::IsTrue(tester.Equals(expected, selected));
		}

		TEST_METHOD(SelectIdealIndependentEmptyTest)
		{
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);
			PosetItems selected = tester.SelectIdealIndependentSelfTest(PosetItems(), D);			
			Assert::IsTrue(tester.Equals(selected, D));
		}

		TEST_METHOD(SelectMaximalTest)
		{
			PosetItems D = list_of<Number>(-2)(5)(-3)(3)(4)(6);

			PosetItems selected = tester.SelectMaximalSelfTest(D);
			PosetItems expected = list_of<Number>(6);

			Assert::IsTrue(tester.Equals(expected, selected));
		}

		TEST_METHOD(SelectMinimalTest)
		{
			PosetItems D = list_of<Number>(-3)(5)(-3)(3)(4)(6);

			PosetItems selected = tester.SelectMinimalSelfTest(D);
			PosetItems expected = list_of<Number>(-3);

			Assert::IsTrue(tester.Equals(expected, selected));
		}

		TEST_METHOD(SelectUniqueTest)
		{
			PosetItems D = list_of<Number>(-3)(5)(-3)(3)(3)(5);

			PosetItems selected = tester.SelectUniqueSelfTest(D);
			PosetItems expected = list_of<Number>(-3)(3)(5);

			Assert::IsTrue(tester.Equals(expected, selected));
		}

		TEST_METHOD(PreceqTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4)(0);			
			tester.PreceqSelfTest(A);
		}

		TEST_METHOD(PrecTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4)(0);			
			tester.PrecSelfTest(A);
		}

		TEST_METHOD(EqualTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4)(0)(3)(-4);			
			tester.EqualSelfTest(A);
		}

		TEST_METHOD(IsMaximalFromTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4)(0)(3)(-4);			
			int r = tester.IsMaximalFromSelfTest(A);
			Assert::AreEqual(1, r);
		}

		TEST_METHOD(IsMinimalFromTest)
		{
			PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4)(0)(3)(-4);			
			int r = tester.IsMinimalFromSelfTest(A);
			Assert::AreEqual(2, r);
		}

		TEST_METHOD(IsItemTest)
		{
			PosetItems A = list_of<Number>(1)(3)(-2)(4)(0)(-4);
			A.push_back(tester.GetPoset()->GetUniversalMaximal());
			A.push_back(tester.GetPoset()->GetUniversalMinimal());
			tester.IsItemSelfTest(A, PosetItems());
			
		}

		TEST_METHOD(ToStringTest)
		{
			PosetItems A = list_of<Number>(1)(3)(-2)(4)(0)(-4);
			A.push_back(tester.GetPoset()->GetUniversalMaximal());
			A.push_back(tester.GetPoset()->GetUniversalMinimal());
			
			tester.ToStringConvertSelfTest(A);
		}

		TEST_METHOD(FromStringTest)
		{
			Strings A = list_of("1")("3")("-2")("4")("0")("-4")("+\\infty")("-\\infty");
			tester.FromStringConvertSelfTest(A);
		}

        TEST_METHOD(GetMinimalTest)
        {
            PosetItems A = list_of<Number>(1)(3)(-2)(4)(0)(-4);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            auto minimal = tester.GetMinimalSelfTest(A);
            PosetItems expected(1, tester.GetPoset()->GetUniversalMinimal());
            Assert::IsTrue(tester.Equals(expected, minimal));
        }

        TEST_METHOD(GetMaximalTest)
        {
            PosetItems A = list_of<Number>(1)(3)(-2)(4)(0)(-4);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            auto maximal = tester.GetMaximalSelfTest(A);
            PosetItems expected(1, tester.GetPoset()->GetUniversalMaximal());
            Assert::IsTrue(tester.Equals(expected, maximal));
        }

        TEST_METHOD(TopologicalSortTest)
        {
            PosetItems A = list_of<Number>(1)(-4)(3)(-2)(4)(0)(3)(-4);			
            auto B = tester.TopologicalSortSelfTest(A);
            PosetItems C = list_of<Number>(4)(3)(3)(1)(0)(-2)(-4)(-4);
            Assert::IsTrue(tester.Equals(B, C));
        }


        TEST_METHOD(PrecImmediateTest)
        {
            PosetItems A = list_of(1)(2)(3)(4)(5)(6)(7);
            //A.push_back(tester.GetPoset()->GetUniversalMinimal());
            //A.push_back(tester.GetPoset()->GetUniversalMaximal());

            tester.PrecImmediateSelfTest(A);
        }

        TEST_METHOD(GetImmediatePrecTest)
        {
            PosetItems A = list_of(1)(2)(3)(6)(7);
            A.push_back(tester.GetPoset()->GetUniversalMinimal());
            A.push_back(tester.GetPoset()->GetUniversalMaximal());

            tester.GetImmediatePrecSelfTest(A);
        }


        TEST_METHOD(GetImmediateSuccTest)
        {
            PosetItems A = list_of(1)(2)(3)(6)(7);
            A.push_back(tester.GetPoset()->GetUniversalMinimal());
            A.push_back(tester.GetPoset()->GetUniversalMaximal());

            tester.GetImmediateSuccSelfTest(A);
        }

        TEST_METHOD(IsMinimalIndependentTest)
        {
            PosetItems candidates=list_of(-1)(-2)(0)(1)(2)(6)(7)(5);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(3)(5);

            tester.IsMinimalIndependentSelfTest(A, candidates);
        }


        TEST_METHOD(GetMinimalIndependentTest)
        {
            PosetItems candidates=list_of(-1)(-2)(0)(1)(2)(6)(7)(5);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(3)(5);
            auto indep =  tester.GetMinimalIndependentSelfTest(A, candidates);

            PosetItems expected = list_of(6);
            Assert::IsTrue(tester.Equals(expected, indep));
        }

        TEST_METHOD(IsMaximalIndependentTest)
        {
            PosetItems candidates=list_of(-1)(-2)(0)(1)(2);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(2)(1);

            tester.IsMaximalIndependentSelfTest(A, candidates);
        }


        TEST_METHOD(GetMaximalIndependentTest)
        {
            PosetItems candidates=list_of(-1)(-2)(0)(1)(2)(3);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(2)(1);

            auto indep = tester.GetMaximalIndependentSelfTest(A, candidates);

            PosetItems expected = list_of(0);
            Assert::IsTrue(tester.Equals(expected, indep));
        }

        TEST_METHOD(GetIndependentIntervalsTest)
        {
            PosetItems candidates = list_of(1)(2)(3)(4)(5)(6)(7);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(4)(6);
            PosetItems B = list_of(1)(0)(2);

            auto indep = tester.GetIndependentIntervalsSelfTest(A, B, candidates);

            Assert::AreEqual(1, indep);
        }

	};
}