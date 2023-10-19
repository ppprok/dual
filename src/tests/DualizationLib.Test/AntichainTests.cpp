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

        TEST_CASE("SelectFilterTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4);
            PosetItems D = list_of(2)(5)(3)(1)(4)(6);

            PosetItems selected = tester.SelectFilterSelfTest(A, D);
            PosetItems expected = list_of(2)(3)(1)(4);

            CHECK(tester.Equals(expected, selected));
        }

        TEST_CASE("SelectFilterEmptyTest)
        {
            PosetItems D = list_of(2)(5)(3)(3)(4)(6);
            PosetItems selected = tester.SelectFilterSelfTest(PosetItems(), D);			
            CHECK(selected.empty());
        }

        TEST_CASE("SelectIdealTest)
        {
            PosetItems A = list_of(1)(1)(3)(0)(4);
            PosetItems D = list_of(2)(5)(7)(3)(4)(6);

            PosetItems selected = tester.SelectIdealSelfTest(A, D);
            PosetItems expected = list_of(3)(4);

            CHECK(tester.Equals(expected, selected));
        }

        TEST_CASE("SelectIdealEmptyTest)
        {
            PosetItems D = list_of(2)(5)(0)(3)(4)(6);
            PosetItems selected = tester.SelectIdealSelfTest(PosetItems(), D);			
            CHECK(selected.empty());
        }

        TEST_CASE("SelectFilterIndependentTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4);
            PosetItems D = list_of(2)(5)(0)(3)(4)(6);

            PosetItems selected = tester.SelectFilterIndependentSelfTest(A, D);
            PosetItems expected = list_of(5)(0)(6);

            CHECK(tester.Equals(expected, selected));
        }

        TEST_CASE("SelectFilterIndependentEmptyTest)
        {
            PosetItems D = list_of(2)(5)(0)(3)(4)(6);
            PosetItems selected = tester.SelectFilterIndependentSelfTest(PosetItems(), D);
            CHECK(tester.Equals(selected, D));
        }

        TEST_CASE("SelectIdealIndependentTest)
        {
            PosetItems A = list_of(1)(2)(3)(0)(4);
            PosetItems D = list_of(2)(5)(8)(3)(4)(6);

            PosetItems selected = tester.SelectIdealIndependentSelfTest(A, D);
            PosetItems expected = list_of(5)(8)(6);

            CHECK(tester.Equals(expected, selected));
        }

        TEST_CASE("SelectIdealIndependentEmptyTest)
        {
            PosetItems D = list_of(2)(5)(3)(3)(4)(6);
            PosetItems selected = tester.SelectIdealIndependentSelfTest(PosetItems(), D);			
            CHECK(tester.Equals(selected, D));
        }

        TEST_CASE("SelectMaximalTest)
        {
            PosetItems D = list_of(2)(5)(3)(3)(4)(6);

            PosetItems selected = tester.SelectMaximalSelfTest(D);
            PosetItems expected = list_of(2)(5)(3)(3)(4)(6);

            CHECK(tester.Equals(expected, selected));
        }

        TEST_CASE("SelectMinimalTest)
        {
            PosetItems D = list_of(0)(5)(3)(4)(6);

            PosetItems selected = tester.SelectMinimalSelfTest(D);
            PosetItems expected = list_of(0)(5)(3)(4)(6);

            CHECK(tester.Equals(expected, selected));
        }

        TEST_CASE("SelectUniqueTest)
        {
            PosetItems D = list_of(0)(5)(0)(3)(3)(5);

            PosetItems selected = tester.SelectUniqueSelfTest(D);
            PosetItems expected = list_of(5)(3)(0);

            CHECK(tester.Equals(expected, selected));
        }

        TEST_CASE("PreceqTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4)(0);			
            tester.PreceqSelfTest(A);
        }

        TEST_CASE("PrecTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(9)(0);			
            tester.PrecSelfTest(A);
        }

        TEST_CASE("EqualTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(8)(0)(3)(9);			
            tester.EqualSelfTest(A);
        }

        TEST_CASE("IsMaximalFromTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4)(0)(3)(4);			
            int r = tester.IsMaximalFromSelfTest(A);
            CHECK_EQ(8, r);
        }

        TEST_CASE("IsMinimalFromTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4)(0)(3)(4);
            int r = tester.IsMinimalFromSelfTest(A);
            CHECK_EQ(8, r);
        }

        TEST_CASE("IsItemTest)
        {
            PosetItems A = list_of(1)(3)(2)(4)(0)(4);
            PosetItems B = list_of(-2)(14)(-1)(10);
            B.push_back(tester.GetPoset()->GetUniversalMaximal());
            B.push_back(tester.GetPoset()->GetUniversalMinimal());
            tester.IsItemSelfTest(A, B);
        }

        TEST_CASE("ToStringTest)
        {
            PosetItems A = list_of(1)(3)(2)(4)(0)(4);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            tester.ToStringConvertSelfTest(A);
        }

        TEST_CASE("FromStringTest)
        {
            Strings A = list_of("1")("3")("2")("4")("0")("9")("+\\infty")("-\\infty");
            tester.FromStringConvertSelfTest(A);
        }

        TEST_CASE("GetMinimalTest)
        {
            PosetItems A = list_of(1)(3)(2)(4)(0)(9);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            auto minimal = tester.GetMinimalSelfTest(A);
            PosetItems expected;
            tester.GetPoset()->GetAllItems(expected);
            CHECK(tester.Equals(expected, minimal));
        }

        TEST_CASE("GetMaximalTest)
        {
            PosetItems A = list_of(1)(3)(2)(4)(0)(9);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            auto maximal = tester.GetMaximalSelfTest(A);
            PosetItems expected;
            tester.GetPoset()->GetAllItems(expected);
            CHECK(tester.Equals(expected, maximal));
        }

        TEST_CASE("TopologicalSortTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4)(0)(3)(4);			
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            
            auto B = tester.TopologicalSortSelfTest(A);            
            PosetItems C = list_of(tester.GetPoset()->GetUniversalMaximal())
                (4)(4)(4)(3)(3)(2)(1)(0);
            CHECK(tester.Equals(B, C));
        }

        
        
        
    };
}