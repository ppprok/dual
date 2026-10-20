#include "stdafx.h"
#include "CppUnitTest.h"
#include "IntegersFileReader.h"
#include "GeneticDualizer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DualizationLibTest
{
	TEST_CLASS(GeneticDualizerTests)
	{
	public:

        struct MatrixRows:
            IRowsSource
        {
            int n;
            bit_chunks uncoveredRows;
            int i;
            
            MatrixRows()
            {
                n = 0;
                i = 0;
            }


            virtual int GetWidth() const
            {
                return n;
            }
                        
            virtual void Reset()
            {
                i = 0;
            }

            virtual void LoadUncoveredRows( bit_chunk fixedColumns, int maxCount, WeightedRows& rows )
            {
                for (int m = uncoveredRows.size(), s = rows.size();i < m && s < maxCount; ++i)
                {
                    if (uncoveredRows[i].intersects(fixedColumns))
                        continue;
                    rows.emplace_back(uncoveredRows[i]);
                    ++s;
                }
            }
        };
		
        // Строки загружаются сразу
        TEST_CASE(" GeneticDualizerMinCoverTest1 )
        {
            GeneticDualizer ga;
            ga.CurrentOptions.mutationScaleScale = 0.01f;
            ga.CurrentOptions.iterationCount = 200;
            ga.CurrentOptions.populationSize = 100;
            bit_matrix mat = IntegersFileReader("../datasets/lose100.hg").ReadBitMatrix();
            
            MatrixRows mrs;
            mrs.n = mat.width();
            auto b = mat.get_rows().begin();
            auto e = mat.get_rows().end();
            mrs.uncoveredRows.assign(b, e);
            //mrs.targetRows.assign(b+50, b+100);
            
            ga.SetRowsSource(&mrs);
            ga.Search();

//            CHECK(ga.GetPopulationSize() <= 15 && ga.GetPopulationSize() > 5);
            CHECK(ga.GetPopulation().front().cost == 1.0f);
            CHECK(ga.GetPopulation().front().cover.count() == 1);

            CHECK(CheckCovers(ga, mat));
        }

        // Строки загружаются постепенно
        TEST_CASE(" GeneticDualizerMinCoverTest2 )
        {
            GeneticDualizer ga;
            ga.CurrentOptions.mutationScaleScale = 0.01f;
            ga.CurrentOptions.iterationCount = 200;
            ga.CurrentOptions.populationSize = 100;
            bit_matrix mat = IntegersFileReader("../datasets/lose800.hg").ReadBitMatrix();

            MatrixRows mrs;
            mrs.n = mat.width();
            auto b = mat.get_rows().begin();
            auto e = mat.get_rows().end();
            mrs.uncoveredRows.assign(b, e);

            ga.SetRowsSource(&mrs);
            ga.Search();

            //CHECK(ga.GetPopulationSize() >= 10);
            CHECK(ga.GetPopulation().front().cost == 1.0f);
            CHECK(ga.GetPopulation().front().cover.count() == 1);

            CHECK(CheckCovers(ga, mat));
        }

        // Линейная интерполяция генов столбцов, т.к. очень много столбцов
        TEST_CASE(" GeneticDualizerMinCoverTest3 )
        {
            GeneticDualizer ga;
            ga.CurrentOptions.mutationScaleScale = 0.01f;
            ga.CurrentOptions.iterationCount = 50;
            ga.CurrentOptions.populationSize = 100;
            bit_matrix mat = IntegersFileReader("../datasets/bms2_800.hg").ReadBitMatrix();

            MatrixRows mrs;
            mrs.n = mat.width();
            auto b = mat.get_rows().begin();
            auto e = mat.get_rows().end();
            mrs.uncoveredRows.assign(b, e);
            //mrs.targetRows.assign(b+30, b+62);

            ga.SetRowsSource(&mrs);
            ga.Search();

            //CHECK_EQ(ga.GetPopulationSize(), 15);
            CHECK(ga.GetPopulation().front().cost == 1.0f);
            CHECK(ga.GetPopulation().front().cover.count() == 1);

            CHECK(CheckCovers(ga, mat));
        }

        // Линейная интерполяция генов столбцов, т.к. очень много столбцов,
        // + отложенная загрузка строк
        TEST_CASE(" GeneticDualizerMinCoverTest4 )
        {
            GeneticDualizer ga;
            ga.CurrentOptions.mutationScaleScale = 0.01f;
            ga.CurrentOptions.iterationCount = 50;
            ga.CurrentOptions.populationSize = 100;
            bit_matrix mat = IntegersFileReader("../datasets/SDTH402.hg").ReadBitMatrix();

            MatrixRows mrs;
            mrs.n = mat.width();
            auto b = mat.get_rows().begin();
            auto e = mat.get_rows().end();
            mrs.uncoveredRows.assign(b, e);
            //mrs.targetRows.assign(b+30, b+62);

            ga.SetRowsSource(&mrs);
            ga.Search();

            //CHECK_EQ(ga.GetPopulationSize(), 15);
            CHECK(ga.GetPopulation().front().cost == ga.GetPopulation().front().cover.count());
            CHECK(ga.GetPopulation().front().cover.count() <= 4);

            CHECK(CheckCovers(ga, mat));
        }

        bool CheckCovers( GeneticDualizer &ga, bit_matrix &mat )
        {
            for (int i = 0; i < ga.GetPopulationSize(); ++i)
            {
                auto& c = ga.GetPopulation()[i].cover;
                for (auto r : mat.get_rows())
                {
                    if  (!r.intersects(c))
                        return false;
                }
            }

            return true;
        }
    };
}