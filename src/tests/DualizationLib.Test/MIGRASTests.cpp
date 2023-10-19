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
    TEST_CLASS(MIGRASTests)
    {
    public:

        std::vector<DualizationAlgorithmOptions> _optionsSet;

        MIGRASTests()
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
            std::string const& filename, 
            std::string const& output, 
            std::string const& config,
            int count = -1)
        {
            int id = 0;

            PosetItemsMatrix results;
            
            for (auto& options : _optionsSet)
            {
                PosetsDualizationTask task;

                UT::Log('\n' + options.Name + '\n');
                                
                auto opt = read_options(config);
                task.SetOptions(opt);
                task.LoadIdealBase(filename, opt);
                        
                DualizationAlgorithm A;
                A.Options = options;
                
                DecisionTreeCSVFileWriter writer;
                
                if (! output.empty())
                {
                    writer.SetFilename(output + "." + options.Name + ".csv");
                    A.ResultCallback.Callback = &writer;
                    A.ResultCallback.Delay = 0;
                    A.InnerCallback.Callback = &writer;
                    A.InnerCallback.Delay = 0;
                }
                
                A.Dualize(task);
                UT::Log(A.Counters.ToString());

                if (count == -1)
                {
                    count = (int)A.Counters.results;
                }
                else
                {
                    CHECK_EQ(count, (int)A.Counters.results);
                }

                if (A.IdealIndependent.GetHeight() > 0)
                {
                    A.IdealIndependent.DeleteDuplicateRows();
                    UT::Log("unique:%1%\n", A.IdealIndependent.GetHeight());
                    CHECK_EQ((int)A.Counters.results, (int)A.IdealIndependent.GetHeight());
                }
                                
                /*if (! results.empty())
                {
                    PosetItemsMatrix diff;
                    std::set_symmetric_difference(
                        A.IdealIndependent.begin(), A.IdealIndependent.end(),
                        results.begin(), results.end(), std::back_inserter(diff),
                        [](PosetItems const& a, PosetItems const& b)
                    {return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());});

                    if (! diff.empty())
                        UT::Log("diff:%1%", diff.size());
                    
                    CHECK(diff.empty());
                }
                else
                {
                    results = std::move(A.IdealIndependent);
                }*/
                
            }
        }

        
        
        
        TEST_CASE("DualizeMIGRASTest1)
        {
            DoTest(
                "../dualizationlib.test/data/A1.csv", 
                "",//"../dualizationlib.test/data/IA1.csv", 
                "../dualizationlib.test/data/dual-posets-task1.info",
                -1);    
        }

        TEST_CASE("DualizeMIGRASTest2)
        {
            DoTest(
                "../dualizationlib.test/data/A2.csv", 
                "",//"../dualizationlib.test/data/IA2.csv", 
                "../dualizationlib.test/data/dual-posets-task1.info",
                -1);    
        }

        TEST_CASE("DualizeMIGRASTest3)
        {
            DoTest(
                "../dualizationlib.test/data/A3.csv", 
                "IA3.csv", 
                "../dualizationlib.test/data/dual-posets-task5.info");    
        }
        
        TEST_CASE("DualizeMIGRASTest4)
        {
            DoTest(
                "../dualizationlib.test/data/A4.csv", 
                "IA4.csv",//"../dualizationlib.test/data/IA4.csv", 
                "../dualizationlib.test/data/dual-posets-task2.info");    
        }
        /*TEST_CASE("DualizeMIGRASTest5)
        {
            DoTest(
                "../dualizationlib.test/data/numberchains-20x10-norm-0-1.csv", 
                "",//"../dualizationlib.test/data/numberchains-20x10-norm-0-1-output.csv", 
                "../dualizationlib.test/data/numberchains-10.json");    
        }*/
       /* TEST_CASE("DualizeMIGRASTest6)
        {
            DoTest(
                "../dualizationlib.test/data/pricegraph-10x10-uniform-10.csv", 
                "",//"../dualizationlib.test/data/pricegraph-10x10-uniform-10-output.csv", 
                "../dualizationlib.test/data/pricegraph-10.json");    
        }*/

        TEST_CASE("DualizeMIGRASTest7)
        {
            DoTest(
                "../dualizationlib.test/data/pricegraph-5x5-uniform-10.csv", 
                "",//"../dualizationlib.test/data/pricegraph-5x5-uniform-10-output.csv", 
                "../dualizationlib.test/data/pricegraph-5.json");    
        }

        
        TEST_CASE("DualizeMIGRASTest8)
        {
            DoTest(
                "../dualizationlib.test/data/A5.csv", 
                "",//"../dualizationlib.test/data/pricegraph-5x5-uniform-10-output.csv", 
                "../dualizationlib.test/data/dual-posets-task3.info");    
        }

        TEST_CASE("DualizeMIGRASTest9)
        {
            DoTest(
                "../dualizationlib.test/data/A6.csv", 
                "",//"../dualizationlib.test/data/pricegraph-5x5-uniform-10-output.csv", 
                "../dualizationlib.test/data/dual-posets-task4.info");    
        }
        
    };
}

