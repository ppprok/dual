#include "stdafx.h"
#include "CppUnitTest.h"
#include "MIGRAS.h"
#include "Counters.h"
#include "NumberChain.h"
#include "GraphPoset.h"
#include "IntervalLattice.h"
#include <iterator>
#include <boost\format.hpp>
#include "UnitTesting.h"
#include "ReversedPoset.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace PosetDualization;
using namespace PosetDualization::MIGRAS;
using namespace UtilsLib;

namespace DualizationLibTest
{
    TEST_CLASS(MIGRASIntervalsTests)
    {
    public:

        std::vector<DualizationAlgorithmOptions> _optionsSet;

        MIGRASIntervalsTests()
        {
            _optionsSet.push_back(DualizationAlgorithmOptions());
            _optionsSet.back().BaseRowSelectionType = DualizationAlgorithmOptions::First;
            _optionsSet.back().Name = "First";
            _optionsSet.back().CollectResultsInMemory = true;

            _optionsSet.push_back(DualizationAlgorithmOptions());
            _optionsSet.back().BaseRowSelectionType = DualizationAlgorithmOptions::MinCoverItemCount;
            _optionsSet.back().Name = "MinRow";
            _optionsSet.back().CollectResultsInMemory = true;

            _optionsSet.push_back(DualizationAlgorithmOptions());
            _optionsSet.back().BaseRowSelectionType = DualizationAlgorithmOptions::None;
            _optionsSet.back().Name = "None";
            _optionsSet.back().CollectResultsInMemory = true;

            _optionsSet.push_back(DualizationAlgorithmOptions());
            _optionsSet.back().BaseRowSelectionType = DualizationAlgorithmOptions::First;
            _optionsSet.back().RandomizeBaseRowSelection = true;
            _optionsSet.back().Name = "RandomRow";
            _optionsSet.back().CollectResultsInMemory = true;

            _optionsSet.push_back(DualizationAlgorithmOptions());
            _optionsSet.back().BaseRowSelectionType = DualizationAlgorithmOptions::MinCoverItemCount;
            _optionsSet.back().RandomizeBaseRowSelection = true;
            _optionsSet.back().ScanRowsLimit = 2;
            _optionsSet.back().Name = "Random5MinRow";
            _optionsSet.back().CollectResultsInMemory = true;

                        
            NumberChain::Register();
            GraphPoset::Register();
            IntervalLattice::Register();
            ReversedPoset::Register();

        }


        void DoTest(
            std::string const& ideal, 
            std::string const& filter, 
            std::string const& config,
            int count = -1)
        {
            PosetsDualizationTask task;


            auto opt = read_options(config);
            task.SetOptions(opt);
            task.LoadIdealBase(ideal, opt);
            task.LoadFilterBase(filter, opt);

            DualizationAlgorithm A;

            A.DualizeIntervalsDirect(task);
            if (count == -1) 
            {
                count = (int)A.Counters.results;
            }
            else
            {
                Assert::AreEqual(count, (int)A.Counters.results);
            }
            //int id = 0;

            //PosetItemsMatrix results;
            
            for (auto& options : _optionsSet)
            {
                UT::Log('\n' + options.Name + '\n');
                        
                DualizationAlgorithm A;
                A.Options = options;
                
                /*DecisionTreeCSVFileWriter writer;                
                if (! output.empty())
                {
                    writer.SetFilename(output + "." + options.Name + ".csv");
                    A.ResultCallback.Callback = &writer;
                    A.ResultCallback.Delay = 0;
                    A.InnerCallback.Callback = &writer;
                    A.InnerCallback.Delay = 0;
                }*/
                
                A.DualizeIntervals(task);
                UT::Log(A.Counters.ToString());

                if (count == -1)
                {
                    count = (int)A.Counters.results;
                }
                else
                {
                    Assert::AreEqual(count, (int)A.Counters.results);
                }

                /*if (! A.IdealIndependent.empty())
                {
                    unique(A.IdealIndependent);
                    UT::Log("unique:%1%\n", A.IdealIndependent.size());
                    Assert::AreEqual((int)A.Counters.results, (int)A.IdealIndependent.size());
                }
                                
                if (! results.empty())
                {
                    PosetItemsMatrix diff;
                    std::set_symmetric_difference(
                        A.IdealIndependent.begin(), A.IdealIndependent.end(),
                        results.begin(), results.end(), std::back_inserter(diff),
                        [](PosetItems const& a, PosetItems const& b)
                    {return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());});

                    if (! diff.empty())
                        UT::Log("diff:%1%", diff.size());
                    
                    Assert::IsTrue(diff.empty());
                }
                else
                {
                    results = std::move(A.IdealIndependent);
                }*/
                
            }
        }

        
        
        
        TEST_METHOD(DualizeIntervalsMIGRASTest1)
        {
            DoTest(
                "../dualizationlib.test/data/A7.csv", 
                "../dualizationlib.test/data/B1.csv", 
                "../dualizationlib.test/data/chaines-3.info",
                -1);    
        }

        TEST_METHOD(DualizeIntervalsMIGRASTest2)
        {
            DoTest(
                "../dualizationlib.test/data/A1.csv", 
                "../dualizationlib.test/data/B1.csv", 
                "../dualizationlib.test/data/dual-posets-task1.info",
                -1);    
        }

        TEST_METHOD(DualizeIntervalsMIGRASMaxResultsTest)
        {
            PosetsDualizationTask task;
            auto opt = read_options("../dualizationlib.test/data/dual-posets-task1.info");
            task.SetOptions(opt);
            task.LoadIdealBase("../dualizationlib.test/data/A1.csv", opt);
            task.LoadFilterBase("../dualizationlib.test/data/B1.csv", opt);

            DualizationAlgorithm A;
            A.Options.CollectResultsInMemory = true;
            A.Options.MaxResults = 10;

            A.DualizeIntervals(task);
            Assert::AreEqual(10, (int)A.Counters.results);            
        }      

        
    };
}

