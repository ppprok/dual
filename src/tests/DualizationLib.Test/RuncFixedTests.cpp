#include <doctest.h>

#include "IntegersFileReader.h"
#include "RuncFixed.h"

TEST_CASE("DualizeRuncFixedTest1") {
    IntegersFileReader reader("../ao.test/datasets/6_6_4.hg");
    auto input = reader.ReadBitMatrix32();
    int r = RuncFixedDualize<bits32>(input);
    CHECK_EQ(4, r);
}

TEST_CASE("DualizeRuncFixedTest2") {
    IntegersFileReader reader("../ao.test/datasets/13_7_10.hg");
    auto input = reader.ReadBitMatrix32();
    int r = RuncFixedDualize(input);
    CHECK_EQ(10, r);
}

TEST_CASE("DualizeRuncFixedTest3") {
    IntegersFileReader reader("../ao.test/datasets/15_10_14.hg");
    auto input = reader.ReadBitMatrix32();
    int r = RuncFixedDualize(input);
    CHECK_EQ(14, r);
}

TEST_CASE("DualizeRuncFixedTest4") {
    IntegersFileReader reader("../ao.test/datasets/15_20_275.hg");
    auto input = reader.ReadBitMatrix32();
    int r = RuncFixedDualize(input);
    CHECK_EQ(275, r);
}

TEST_CASE("DualizeRuncFixedTest5") {
    IntegersFileReader reader("../ao.test/datasets/10_50_7187.hg");
    auto input = reader.ReadBitMatrix64();

    RuncFixed<bits64> runc;
    runc.Dualize(input);
    CHECK_EQ(7187, (int) runc.idealIndep.size());

    RuncFixed<bits64> runc1;
    runc1.Dualize(runc.idealIndep);
    CHECK_EQ(10, (int) runc1.idealIndep.size());
}

TEST_CASE("DualizeRuncFixedTest6") {
    IntegersFileReader reader("../ao.test/datasets/15_35_12226.hg");
    auto input = reader.ReadBitMatrix64();
    auto r = RuncFixedDualize(input);
    CHECK_EQ(12226, (int) r);
}

TEST_CASE("DualizeRuncFixedDualTest1") {
    IntegersFileReader reader("../ao.test/datasets/10_50_7187.hg");
    auto input = reader.ReadBitMatrix64();

    RuncFixed<bits64> runc;
    runc.DualizeIntervals(input, std::vector<bits64>());
    auto input1 = runc.idealIndep;
    CHECK_EQ(7187, (int) input1.size());

    input1.resize(20);

    RuncFixed<bits64> runc1;
    runc1.DualizeIntervals(input, input1);

    unique(runc1.idealIndep);

    CHECK_EQ(7167, (int) runc1.idealIndep.size());
}

TEST_CASE("DualizeRuncFixedDualTest2") {
    IntegersFileReader reader("../ao.test/datasets/13_7_10.hg");
    auto input = reader.ReadBitMatrix32();

    RuncFixed<bits32> runc;
    runc.Dualize(input);
    CHECK_EQ(10, (int) runc.idealIndep.size());

    auto input1 = runc.idealIndep;
    input1.resize(5);

    RuncFixed<bits32> runc1;
    runc1.DualizeIntervals(input, input1);

    unique(runc1.idealIndep);

    CHECK_EQ(5, (int) runc1.idealIndep.size());
}

TEST_CASE("DualizeRuncFixedDualTest3") {
    IntegersFileReader reader("../ao.test/datasets/6_6_4.hg");
    auto input = reader.ReadBitMatrix32();

    RuncFixed<bits32> runc;
    runc.Dualize(input);
    CHECK_EQ(4, (int) runc.idealIndep.size());

    std::vector<bits32> input1(runc.idealIndep.begin() + 1, runc.idealIndep.begin() + 2);

    RuncFixed<bits32> runc1;
    runc1.DualizeIntervals(input, input1);
    unique(runc1.idealIndep);
    CHECK_EQ(3, (int) runc1.idealIndep.size());
}
