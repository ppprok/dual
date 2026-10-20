#include "stdafx.h"
#include "CppUnitTest.h"
#include "GraphPoset.h"
#include "PosetSelfTester.h"
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DualizationLibTest
{
    TEST_CLASS(GraphPosetTests)
    {
    
        PosetSelfTester tester;

    public:

        GraphPosetTests()
            :tester(std::make_shared<GraphPoset>())
        {
            auto graphPoset = std::dynamic_pointer_cast<GraphPoset>(tester.GetPoset());

            DirectedEdges edges;
            edges.emplace_back(0,2);
            edges.emplace_back(2,3);
            edges.emplace_back(1,2);
            edges.emplace_back(1,3);
            edges.emplace_back(2,4);
            edges.emplace_back(5,4);
            edges.emplace_back(4,6);

            graphPoset->SetGraph(7, edges);
        }

        TEST_CASE("SetOptionsTest)
        {
            auto options = read_json_options("../DualizationLib.Test/data/graphposet1.json");
            GraphPoset poset;
            poset.SetOptions(options);

            CHECK_EQ(7, poset.GetSize());
            PosetItems items;
            poset.GetAllItems(items);

            for(auto& a:items)
                for (auto& b: items)
                    CHECK_EQ(poset.Preceq(a, b), tester.GetPoset()->Preceq(a, b));

        }
        

        TEST_CASE("SelectFilterTest)
        {
            PosetItems A = list_of(1)(4)(2)(4);
            PosetItems D = list_of(2)(5)(3)(1)(4)(6);

            PosetItems selected = tester.SelectFilterSelfTest(A, D);
            PosetItems expected = list_of(2)(5)(1)(4);

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
            PosetItems expected = list_of(2)(7)(3)(4)(6);

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
            PosetItems A = list_of(1)(4)(2)(4);
            PosetItems D = list_of(2)(5)(0)(3)(4)(6);

            PosetItems selected = tester.SelectFilterIndependentSelfTest(A, D);
            PosetItems expected = list_of(3)(6);

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
            PosetItems A = list_of(1)(1)(3)(0)(4);
            PosetItems D = list_of(2)(5)(7)(3)(4)(6);

            PosetItems selected = tester.SelectIdealIndependentSelfTest(A, D);
            PosetItems expected = list_of(5);

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
            PosetItems D = list_of(2)(5)(3)(3)(4);

            PosetItems selected = tester.SelectMaximalSelfTest(D);
            PosetItems expected = list_of(3)(3)(4);

            CHECK(tester.Equals(expected, selected));
        }

        TEST_CASE("SelectMinimalTest)
        {
            PosetItems D = list_of(6)(3)(4)(6);

            PosetItems selected = tester.SelectMinimalSelfTest(D);
            PosetItems expected = list_of(3)(4);

            CHECK(tester.Equals(expected, selected));
        }

        TEST_CASE("SelectUniqueTest)
        {
            PosetItems D = list_of(0)(5)(0)(3)(3)(5);

            PosetItems selected = tester.SelectUniqueSelfTest(D);
            PosetItems expected = list_of(0)(3)(5);

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
            CHECK_EQ(5, r);
        }

        TEST_CASE("IsMinimalFromTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4)(0)(3)(4);
            int r = tester.IsMinimalFromSelfTest(A);
            CHECK_EQ(2, r);
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
            PosetItems expected = list_of(0)(1)(5);
            CHECK(tester.Equals(expected, minimal));
        }

        TEST_CASE("GetMaximalTest)
        {
            PosetItems A = list_of(1)(3)(2)(4)(0)(9);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            auto maximal = tester.GetMaximalSelfTest(A);
            PosetItems expected=list_of(3)(6);
            CHECK(tester.Equals(expected, maximal));
        }

        TEST_CASE("TopologicalSortTest)
        {
            PosetItems A = list_of(1)(4)(3)(2)(4)(0)(3)(4);			
            A.push_back(tester.GetPoset()->GetUniversalMaximal());

            auto B = tester.TopologicalSortSelfTest(A);            
            PosetItems C = list_of(tester.GetPoset()->GetUniversalMaximal())
                (3)(3)(4)(4)(4)(2)(0)(1);
            CHECK(tester.Equals(B, C));
        }


        TEST_CASE("PrecImmediateTest)
        {
            PosetItems A;
            tester.GetPoset()->GetAllItems(A);
            A.push_back(tester.GetPoset()->GetUniversalMinimal());
            A.push_back(tester.GetPoset()->GetUniversalMaximal());

            tester.PrecImmediateSelfTest(A);
        }

        TEST_CASE("GetImmediatePrecTest)
        {
            PosetItems A;
            tester.GetPoset()->GetAllItems(A);
            A.push_back(tester.GetPoset()->GetUniversalMinimal());
            A.push_back(tester.GetPoset()->GetUniversalMaximal());

            tester.GetImmediatePrecSelfTest(A);
        }


        TEST_CASE("GetImmediateSuccTest)
        {
            PosetItems A;
            tester.GetPoset()->GetAllItems(A);
            A.push_back(tester.GetPoset()->GetUniversalMinimal());
            A.push_back(tester.GetPoset()->GetUniversalMaximal());

            tester.GetImmediateSuccSelfTest(A);
        }

        TEST_CASE("IsMinimalIndependentTest)
        {
            PosetItems candidates;
            tester.GetPoset()->GetAllItems(candidates);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(3)(5);

            tester.IsMinimalIndependentSelfTest(A, candidates);
        }


        TEST_CASE("GetMinimalIndependentTest)
        {
            PosetItems candidates;
            tester.GetPoset()->GetAllItems(candidates);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(3)(5);
            auto indep =  tester.GetMinimalIndependentSelfTest(A, candidates);

            PosetItems expected = list_of(4);
            CHECK(tester.Equals(expected, indep));
        }

        TEST_CASE("IsMaximalIndependentTest)
        {
            PosetItems candidates;
            tester.GetPoset()->GetAllItems(candidates);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(2)(1);

            tester.IsMaximalIndependentSelfTest(A, candidates);
        }


        TEST_CASE("GetMaximalIndependentTest)
        {
            PosetItems candidates;
            tester.GetPoset()->GetAllItems(candidates);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(2)(1);

            auto indep = tester.GetMaximalIndependentSelfTest(A, candidates);

            PosetItems expected = list_of(0)(5);
            CHECK(tester.Equals(expected, indep));
        }

        TEST_CASE("GetIndependentIntervalsTest)
        {
            PosetItems candidates;
            tester.GetPoset()->GetAllItems(candidates);
            candidates.push_back(tester.GetPoset()->GetUniversalMinimal());
            candidates.push_back(tester.GetPoset()->GetUniversalMaximal());

            PosetItems A = list_of(6);
            PosetItems B = list_of(5);

            auto indep = tester.GetIndependentIntervalsSelfTest(A, B, candidates);

            CHECK_EQ(4, indep);
        }


    };
}


