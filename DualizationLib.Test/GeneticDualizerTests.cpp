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
        TEST_METHOD( GeneticDualizerMinCoverTest1 )  
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

//            Assert::IsTrue(ga.GetPopulationSize() <= 15 && ga.GetPopulationSize() > 5);
            Assert::IsTrue(ga.GetPopulation().front().cost == 1.0f);
            Assert::IsTrue(ga.GetPopulation().front().cover.count() == 1);

            Assert::IsTrue(CheckCovers(ga, mat));
        }

        // Строки загружаются постепенно
        TEST_METHOD( GeneticDualizerMinCoverTest2 )  
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

            //Assert::IsTrue(ga.GetPopulationSize() >= 10);
            Assert::IsTrue(ga.GetPopulation().front().cost == 1.0f);
            Assert::IsTrue(ga.GetPopulation().front().cover.count() == 1);

            Assert::IsTrue(CheckCovers(ga, mat));
        }

        // Линейная интерполяция генов столбцов, т.к. очень много столбцов
        TEST_METHOD( GeneticDualizerMinCoverTest3 )  
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

            //Assert::AreEqual(ga.GetPopulationSize(), 15);
            Assert::IsTrue(ga.GetPopulation().front().cost == 1.0f);
            Assert::IsTrue(ga.GetPopulation().front().cover.count() == 1);

            Assert::IsTrue(CheckCovers(ga, mat));
        }

        // Линейная интерполяция генов столбцов, т.к. очень много столбцов,
        // + отложенная загрузка строк
        TEST_METHOD( GeneticDualizerMinCoverTest4 )  
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

            //Assert::AreEqual(ga.GetPopulationSize(), 15);
            Assert::IsTrue(ga.GetPopulation().front().cost == ga.GetPopulation().front().cover.count());
            Assert::IsTrue(ga.GetPopulation().front().cover.count() <= 4);

            Assert::IsTrue(CheckCovers(ga, mat));
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