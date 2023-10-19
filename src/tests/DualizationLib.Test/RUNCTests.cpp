#include <doctest.h>

#include "Counters.h"
#include "RUNC.h"

void DoTest(std::string const& filename, int count) {
    Runc::AlgorithmType algs[] = {Runc::RUNC, Runc::OPT, Runc::RUNCM};

    for (auto a : algs) {
        Runc::GlobalState A(a);
        A.LoadMatrix(filename);
        Counters c;
        A.InnerCallback = &c;
        A.CoverCallback = &c;
        A.Dualize();
        CHECK_EQ(count, c.cover);
    }
}

TEST_CASE("DualizeRUNCTest1") {
    DoTest("data/6_6_4.hg", 4);
}

TEST_CASE("DualizeRUNCTest2") {
    DoTest("data/13_7_10.hg", 10);
}

TEST_CASE("DualizeRUNCTest3") {
    DoTest("data/15_10_14.hg", 14);
}

TEST_CASE("DualizeRUNCTest4") {
    DoTest("data/15_20_275.hg", 275);
}

TEST_CASE("DualizeRUNCTest5") {
    DoTest("data/10_25_480.hg", 480);
}

TEST_CASE("DualizeRUNCTest6") {
    DoTest("data/10_50_7187.hg", 7187);
}

TEST_CASE("DualizeRUNCTest7") {
    DoTest("data/15_35_12226.hg", 12226);
}
