#include "stdafx.h"

#include "IntervalLattice.h"
#include "PosetSelfTester.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DualizationLibTest
{
    TEST_CLASS(IntervalLatticeTests)
    {
    public:

        PosetSelfTester tester;

        IntervalLatticeTests()
            :tester(std::make_shared<IntervalLattice>(-15,15))
        {
        }

        PosetItems GenerateIntervals(int min, int max)
        {
            auto umax = tester.GetPoset()->GetUniversalMaximal();
            
            PosetItems intervals;
            for (int i = min; i <= max-1; ++i)
            {
                for (int j = i+1; j <= max; ++j)
                    intervals.emplace_back(i,j);

                intervals.emplace_back(umax.value.interval.min, i);
                intervals.emplace_back(i, umax.value.interval.max);
            }

            intervals.emplace_back(0,0);
            intervals.emplace_back(umax);
            return intervals;
        }

        TEST_METHOD(SelectFilterTest)
        {
            PosetItems A = list_of<PosetItem>(1,2)(-4,-2)(3,5)(-2,7)(4,9);
            PosetItems D = list_of<PosetItem>(-3,3)(-5,-2)(3,5)(-1,6)(0,0);//GenerateIntervals(-4, 4);

            PosetItems selected = tester.SelectFilterSelfTest(A, D);
            PosetItems expected = list_of<PosetItem>(3,5)(-1,6)(0,0);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(SelectFilterEmptyTest)
        {
            PosetItems D = list_of<PosetItem>(1,2)(-4,-2)(3,5)(-2,7)(4,9);
            PosetItems selected = tester.SelectFilterSelfTest(PosetItems(), D);			
            Assert::IsTrue(selected.empty());
        }

        TEST_METHOD(SelectIdealTest)
        {
            PosetItems A = list_of<PosetItem>(1,2)(-4,-2)(3,5)(-2,7)(4,9);
            PosetItems D = list_of<PosetItem>(-3,3)(-5,-2)(3,5)(-1,6)(0,0)(3,4);//GenerateIntervals(-4, 4);

            PosetItems selected = tester.SelectIdealSelfTest(A, D);
            PosetItems expected = list_of<PosetItem>(-3,3)(-5,-2)(3,5)(-1,6);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(SelectIdealEmptyTest)
        {
            PosetItems D = list_of<PosetItem>(1,2)(-4,-2)(3,5)(-2,7)(4,9);
            PosetItems selected = tester.SelectIdealSelfTest(PosetItems(), D);			
            Assert::IsTrue(selected.empty());
        }

        TEST_METHOD(SelectFilterIndependentTest)
        {
            PosetItems A = list_of<PosetItem>(1,2)(-4,-2)(3,5)(-2,7)(4,9);
            PosetItems D = list_of<PosetItem>(-3,3)(-5,-2)(3,5)(-1,6)(0,0);//GenerateIntervals(-4, 4);

            PosetItems selected = tester.SelectFilterIndependentSelfTest(A, D);
            PosetItems expected = list_of<PosetItem>(-3,3)(-5,-2);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(SelectFilterIndependentEmptyTest)
        {
            PosetItems D = list_of<PosetItem>(-3,3)(-5,-2)(3,5)(-1,6)(0,0);
            PosetItems selected = tester.SelectFilterIndependentSelfTest(PosetItems(), D);			
            Assert::IsTrue(tester.Equals(selected, D));
        }

        TEST_METHOD(SelectIdealIndependentTest)
        {
            PosetItems A = list_of<PosetItem>(1,2)(-4,-2)(3,5)(-2,7)(4,9);
            PosetItems D = list_of<PosetItem>(-3,3)(-5,-2)(3,5)(-1,6)(0,0)(3,4);//GenerateIntervals(-4, 4);

            PosetItems selected = tester.SelectIdealIndependentSelfTest(A, D);
            PosetItems expected = list_of<PosetItem>(0,0)(3,4);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(SelectIdealIndependentEmptyTest)
        {
            PosetItems D = list_of<PosetItem>(-3,3)(-5,-2)(3,5)(-1,6)(0,0)(3,4);//GenerateIntervals(-4, 4);
            PosetItems selected = tester.SelectIdealIndependentSelfTest(PosetItems(), D);			
            Assert::IsTrue(tester.Equals(selected, D));
        }

        TEST_METHOD(SelectMaximalTest)
        {
            PosetItems D = list_of<PosetItem>(-2,10)(5,6)(-1,0)(3,7)(4,5)(2,12);

            PosetItems selected = tester.SelectMaximalSelfTest(D);
            PosetItems expected = list_of<PosetItem>(-2,10)(2,12);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(SelectMinimalTest)
        {
            PosetItems D = list_of<PosetItem>(-3,4)(-2,2)(-1,0)(-2,5)(-2,2);

            PosetItems selected = tester.SelectMinimalSelfTest(D);
            PosetItems expected = list_of<PosetItem>(-1,0);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(SelectUniqueTest)
        {
            PosetItems D = list_of<PosetItem>(-3,-3)(5,6)(1,1)(5,6)(1,2)(1,2);

            PosetItems selected = tester.SelectUniqueSelfTest(D);
            PosetItems expected = list_of<PosetItem>(0,0)(1,2)(5,6);

            Assert::IsTrue(tester.Equals(expected, selected));
        }

        TEST_METHOD(PreceqTest)
        {
            PosetItems A = GenerateIntervals(-4,4);
            tester.PreceqSelfTest(A);
        }

        TEST_METHOD(PrecTest)
        {
            PosetItems A = GenerateIntervals(-4,4);
            tester.PrecSelfTest(A);
        }

        TEST_METHOD(EqualTest)
        {
            PosetItems A = GenerateIntervals(-4,4);
            tester.EqualSelfTest(A);
        }

        TEST_METHOD(IsMaximalFromTest)
        {
            PosetItems A = list_of<PosetItem>(-2,10)(5,6)(-1,0)(3,7)(4,5)(2,12);
            int r = tester.IsMaximalFromSelfTest(A);
            Assert::AreEqual(2, r);
        }

        TEST_METHOD(IsMinimalFromTest)
        {
            PosetItems A = list_of<PosetItem>(-3,4)(-2,2)(-1,0)(-2,5)(-2,2);
            int r = tester.IsMinimalFromSelfTest(A);
            Assert::AreEqual(1, r);
        }

        TEST_METHOD(IsItemTest)
        {
            PosetItems A = list_of<PosetItem>(1,1)(3,4)(-2,15)(4,14)(0,9)(-4,-3);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());
            
            PosetItems B = list_of<PosetItem>(2,1)(-2,-555)(-16,7);

            tester.IsItemSelfTest(A, PosetItems());
        }

        TEST_METHOD(ToStringTest)
        {
            PosetItems A = list_of<PosetItem>(1,3)(3,4)(-2,3)(4,6)(0,10)(-4,6);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            tester.ToStringConvertSelfTest(A);
        }

        TEST_METHOD(FromStringTest)
        {
            Strings A = list_of("[1:4]")("[3:12]")("[-2:15]")("[4:6]")("[0:1]")("\\emptyset")("[-15:15]");
            tester.FromStringConvertSelfTest(A);
        }

        TEST_METHOD(GetMinimalTest)
        {
            PosetItems A = list_of<PosetItem>(1,2)(3,6)(-2,-1)(4,5)(0,10)(-4,5);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            auto minimal = tester.GetMinimalSelfTest(A);
            PosetItems expected(1, tester.GetPoset()->GetUniversalMinimal());
            Assert::IsTrue(tester.Equals(expected, minimal));
        }

        TEST_METHOD(GetMaximalTest)
        {
            PosetItems A = list_of<PosetItem>(1,2)(3,6)(-2,-1)(4,5)(0,10)(-4,5);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());
            A.push_back(tester.GetPoset()->GetUniversalMinimal());

            auto maximal = tester.GetMaximalSelfTest(A);
            PosetItems expected(1, tester.GetPoset()->GetUniversalMaximal());
            Assert::IsTrue(tester.Equals(expected, maximal));
        }

        TEST_METHOD(TopologicalSortTest)
        {
            PosetItems A = list_of<PosetItem>(1,2)(3,6)(-2,-1)(4,5)(0,10)(-4,5);
            auto B = tester.TopologicalSortSelfTest(A);
            tester.WriteLog("B=").WriteLog(B).WriteLog("\n");            
            PosetItems C = list_of<PosetItem>(0,10)(-4,5)(1,2)(3,6)(-2,-1)(4,5);
            Assert::IsTrue(tester.Equals(B, C));
        }


        TEST_METHOD(PrecImmediateTest)
        {
            PosetItems A = GenerateIntervals(-3,3);
            //tester.GetPoset()->GetAllItems(A);
            
            tester.PrecImmediateSelfTest(A);
        }

        TEST_METHOD(GetImmediatePrecTest)
        {
            PosetItems A = list_of<PosetItem>(1,4)(-2,0)(3,3)(-7,-4);
            A.push_back(tester.GetPoset()->GetUniversalMinimal());
            A.push_back(tester.GetPoset()->GetUniversalMaximal());

            tester.GetImmediatePrecSelfTest(A);
        }


        TEST_METHOD(GetImmediateSuccTest)
        {
            PosetItems A = list_of<PosetItem>(1,4)(-2,0)(2,3)(-7,-4);
            A.push_back(tester.GetPoset()->GetUniversalMaximal());

            tester.GetImmediateSuccSelfTest(A);
        }

        TEST_METHOD(IsMinimalIndependentTest)
        {
            PosetItems candidates=GenerateIntervals(-5,5);

            PosetItems A = list_of<PosetItem>(1,2)(3,5)(-2,-1)(4,5)(0,5)(-4,5);

            tester.IsMinimalIndependentSelfTest(A, candidates);
        }


        TEST_METHOD(GetMinimalIndependentTest)
        {
            PosetItems candidates;
            
            tester.GetPoset()->GetAllItems(candidates);//GenerateIntervals(-10,10);
            
            PosetItems A = list_of<PosetItem>(-7,-2)(3,5)(-2,1)(3,7)(0,5)(-4,5);
            auto indep =  tester.GetMinimalIndependentSelfTest(A, candidates);
            tester.WriteLog("indep=").WriteLog(indep).WriteLog("\n");

            tester.GetPoset()->SelectFilter(list_of<PosetItem>(-8,8), indep);

            /*Assert::AreEqual(4u, indep.size());
            Assert::IsTrue(contains(indep, PosetItem(-5,-1)));
            Assert::IsTrue(contains(indep, PosetItem(2,6)));*/

            PosetItems expected = list_of<PosetItem>(-8,-7)(-5,-1)(2,6)(7,8);
            Assert::IsTrue(tester.Equals(indep, expected));
        }

        TEST_METHOD(IsMaximalIndependentTest)
        {
            PosetItems candidates=GenerateIntervals(-5,5);
            candidates.pop_back();
            candidates.pop_back();

            PosetItems A = list_of<PosetItem>(1,2)(3,5)(-2,-1)(4,5)(0,5)(-4,5);

            tester.IsMaximalIndependentSelfTest(A, candidates);
        }


        TEST_METHOD(GetMaximalIndependentTest)
        {
            PosetItems candidates=GenerateIntervals(-5,5);
            candidates.pop_back();
            candidates.pop_back();

            PosetItems A = list_of<PosetItem>(1,2)(3,5)(-2,-1)(4,5)(0,5)(-4,5);

            auto indep = tester.GetMaximalIndependentSelfTest(A, candidates);
            //auto inf = std::numeric_limits<PosetItem::IntervalBound>::max();
            PosetItems expected = list_of<PosetItem>(-15,-2)(-1,1)(2,4)(5,15);
            Assert::IsTrue(tester.Equals(expected, indep));
        }

        TEST_METHOD(GetMaximalIndependent2Test)
        {
            PosetItems candidates=GenerateIntervals(-5,5);
            candidates.pop_back();
            candidates.pop_back();

            PosetItems A = list_of<PosetItem>(-7,-4)(-3,2)(0,3)(1,3)(1,6)(3,9)(10,14);

            auto indep = tester.GetMaximalIndependentSelfTest(A, candidates);
            //auto inf = std::numeric_limits<PosetItem::IntervalBound>::max();
            PosetItems expected = list_of<PosetItem>(-15,-5)(-6,1)(-2,2)(2,8)(4,13)(11,15);
            Assert::IsTrue(tester.Equals(expected, indep));
        }

        TEST_METHOD(GetIndependentIntervalsTest)
        {
            PosetItems candidates = GenerateIntervals(-10,10);

            PosetItems A = list_of<PosetItem>(1,2)(3,5)(-2,-1)(4,5)(0,5)(-4,5);
            PosetItems B = list_of<PosetItem>(-7,-2)(3,5)(-2,1)(3,7)(0,5)(-4,5);

            auto indep = tester.GetIndependentIntervalsSelfTest(A, B, candidates);
            

            Assert::AreEqual(16, indep);
        }


    };
}



