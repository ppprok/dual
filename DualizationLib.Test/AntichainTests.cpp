#include "stdafx.h"


#include "CppUnitTest.h"
#include "GraphPoset.h"
#include "PosetsDualizationTask.h"
#include "PosetSelfTester.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DualizationLibTest
{
    TEST_CLASS(AntichainTests)
    {

    public:

        PosetSelfTester tester;

        AntichainTests()
            :tester(std::make_shared<Antichain>(10))
        {
        }

        TEST_METHOD(SelectFilterTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4);
            PosetItems D = list_of(2)(5)(3)(1)(4)(6);

            PosetItems selected = tester.SelectFilterSelfTest(A, D);
            PosetItems expected = list_of(2)(3)(1)(4);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(SelectFilterEmptyTest)
        {
            PosetItems D = list_of(2)(5)(3)(3)(4)(6);
            PosetItems selected = tester.SelectFilterSelfTest(PosetItems(), D);			
            Assert::IsTrue(selected.empty());
        }

        TEST_METHOD(SelectIdealTest)
        {
            PosetItems A = list_of(1)(1)(3)(0)(4);
            PosetItems D = list_of(2)(5)(7)(3)(4)(6);

            PosetItems selected = tester.SelectIdealSelfTest(A, D);
            PosetItems expected = list_of(3)(4);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(SelectIdealEmptyTest)
        {
            PosetItems D = list_of(2)(5)(0)(3)(4)(6);
            PosetItems selected = tester.SelectIdealSelfTest(PosetItems(), D);			
            Assert::IsTrue(selected.empty());
        }

        TEST_METHOD(SelectFilterIndependentTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4);
            PosetItems D = list_of(2)(5)(0)(3)(4)(6);

            PosetItems selected = tester.SelectFilterIndependentSelfTest(A, D);
            PosetItems expected = list_of(5)(0)(6);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(SelectFilterIndependentEmptyTest)
        {
            PosetItems D = list_of(2)(5)(0)(3)(4)(6);
            PosetItems selected = tester.SelectFilterIndependentSelfTest(PosetItems(), D);
            Assert::IsTrue(tester.Equals(selected, D));
        }

        TEST_METHOD(SelectIdealIndependentTest)
        {
            PosetItems A = list_of(1)(2)(3)(0)(4);
            PosetItems D = list_of(2)(5)(8)(3)(4)(6);

            PosetItems selected = tester.SelectIdealIndependentSelfTest(A, D);
            PosetItems expected = list_of(5)(8)(6);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(SelectIdealIndependentEmptyTest)
        {
            PosetItems D = list_of(2)(5)(3)(3)(4)(6);
            PosetItems selected = tester.SelectIdealIndependentSelfTest(PosetItems(), D);			
            Assert::IsTrue(tester.Equals(selected, D));
        }

        TEST_METHOD(SelectMaximalTest)
        {
            PosetItems D = list_of(2)(5)(3)(3)(4)(6);

            PosetItems selected = tester.SelectMaximalSelfTest(D);
            PosetItems expected = list_of(2)(5)(3)(3)(4)(6);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(SelectMinimalTest)
        {
            PosetItems D = list_of(0)(5)(3)(4)(6);

            PosetItems selected = tester.SelectMinimalSelfTest(D);
            PosetItems expected = list_of(0)(5)(3)(4)(6);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(SelectUniqueTest)
        {
            PosetItems D = list_of(0)(5)(0)(3)(3)(5);

            PosetItems selected = tester.SelectUniqueSelfTest(D);
            PosetItems expected = list_of(5)(3)(0);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(PreceqTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4)(0);			
            tester.PreceqSelfTest(A);
        }

        TEST_METHOD(PrecTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(9)(0);			
            tester.PrecSelfTest(A);
        }

        TEST_METHOD(EqualTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(8)(0)(3)(9);			
            tester.EqualSelfTest(A);
        }

        TEST_METHOD(IsMaximalFromTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4)(0)(3)(4);			
            int r = tester.IsMaximalFromSelfTest(A);
            Assert::AreEqual(8, r);
        }

        TEST_METHOD(IsMinimalFromTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4)(0)(3)(4);
            int r = tester.IsMinimalFromSelfTest(A);
            Assert::AreEqual(8, r);
        }

        TEST_METHOD(IsItemTest)
        {
            PosetItems A = list_of(1)(3)(2)(4)(0)(4);
            PosetItems B = list_of(-2)(14)(-1)(10);
            B.push_back(tester.GetPoset()->GetUniversalMaximal());
            B.push_back(tester.GetPoset()->GetUniversalMinimal());
            tester.IsItemSelfTest(A, B);
        }

        TEST_METHOD(ToStringTest)
        {
            PosetItems A = list_of(1)(3)(2)(4)(0)(4);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            tester.ToStringConvertSelfTest(A);
        }

        TEST_METHOD(FromStringTest)
        {
            Strings A = list_of("1")("3")("2")("4")("0")("9")("+\\infty")("-\\infty");
            tester.FromStringConvertSelfTest(A);
        }

        TEST_METHOD(GetMinimalTest)
        {
            PosetItems A = list_of(1)(3)(2)(4)(0)(9);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            auto minimal = tester.GetMinimalSelfTest(A);
            PosetItems expected;
            tester.GetPoset()->GetAllItems(expected);
            Assert::IsTrue(tester.Equals(expected, minimal));
        }

        TEST_METHOD(GetMaximalTest)
        {
            PosetItems A = list_of(1)(3)(2)(4)(0)(9);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            auto maximal = tester.GetMaximalSelfTest(A);
            PosetItems expected;
            tester.GetPoset()->GetAllItems(expected);
            Assert::IsTrue(tester.Equals(expected, maximal));
        }

        TEST_METHOD(TopologicalSortTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4)(0)(3)(4);			
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            
            auto B = tester.TopologicalSortSelfTest(A);            
            PosetItems C = list_of(tester.GetPoset()->GetUniversalMaximal())
                (4)(4)(4)(3)(3)(2)(1)(0);
            Assert::IsTrue(tester.Equals(B, C));
        }

        
        
        
    };
}