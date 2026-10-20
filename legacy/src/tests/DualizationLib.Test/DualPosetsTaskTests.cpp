#include "stdafx.h"

#include <boost\property_tree\info_parser.hpp>

#include "CppUnitTest.h"
#include "GraphPoset.h"
#include "PosetsDualizationTask.h"
#include "NumberChain.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DualizationLibTest
{
    TEST_CLASS(DualPosetsTaskTests)
    {

    public:

        TEST_CASE("SetOptionsTests)
        {
            NumberChain::Register();
            GraphPoset::Register();

            PosetsDualizationTask task;
            
            Options opt;
            boost::property_tree::read_info("../dualizationlib.test/data/dual-posets-task1.info", opt);
            
            task.SetOptions(opt);
            task.LoadIdealBase("../dualizationlib.test/data/A1.csv", opt);

            CHECK_EQ(3u, task.GetPosetsProduct().GetPosets().size());
            auto const& ctask = task;
            for (int i = 0; i < 3; ++i)
            {
                CHECK_EQ(16u, ctask.GetIdealBase().GetColumn(i).size());
            }
                        
        }



    };
}