#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\UtilsLib\CSVLoader.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UtilsLibTest
{
    TEST_CLASS(CSVTests)
    {
    public:


        TEST_METHOD(load_CSV_and_create_columns)
        {
            CSVLoader csv;
            csv.Load("../UtilsLib.Test/data/winequality-red.csv");
            csv.CreateColumns(
                [](char c){return c == ';';},                
                [](char c){return c == '\n';});
            csv.TrimSpaces(CharMap(std::string("\"\t \r")));

            Assert::AreEqual((int)csv.GetColumns().size(), 12);
            Assert::IsTrue(csv.GetColumns()[3].GetName() == std::string("residual sugar"));
            Assert::AreEqual(csv.GetColumns()[4].GetValue<double>(4), 0.075);

            for (int i = 0; i < 12; ++i)
            {
                Assert::AreEqual(1600, (int) csv.GetColumns()[i].values.size());
            }

            Assert::AreEqual(csv.GetColumns()[11].GetValue<int>(1599), 6);
        }
    };
}