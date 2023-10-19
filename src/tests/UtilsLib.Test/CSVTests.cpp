#include <doctest.h>

#include "CSVLoader.h"

TEST_CASE("load_CSV_and_create_columns") {
    CSVLoader csv;
    csv.Load("data/winequality-red.csv");
    csv.CreateColumns([](char c) { return c == ';'; }, [](char c) { return c == '\n'; });
    csv.TrimSpaces(CharMap(std::string("\"\t \r")));

    REQUIRE_EQ((int) csv.GetColumns().size(), 12);
    CHECK(csv.GetColumns()[3].GetName() == std::string("residual sugar"));
    CHECK_EQ(csv.GetColumns()[4].GetValue<double>(4), 0.075);

    for (int i = 0; i < 12; ++i) {
        CHECK_EQ(1600, (int) csv.GetColumns()[i].values.size());
    }

    CHECK_EQ(csv.GetColumns()[11].GetValue<int>(1599), 6);
}
