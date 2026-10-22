#include <doctest.h>

#include <dual/counters.h>
#include <dual/runc.h>

using namespace dual;

void DoTest(std::string const& filename, int count) {
    Runc::AlgorithmType algs[] = {Runc::RUNC, Runc::OPT, Runc::RUNCM};

    for (auto a : algs) {
        Runc::RuncGlobalState A(a);
        A.loadMatrix(filename);
        Counters c;
        A.InnerCallback = &c;
        A.CoverCallback = &c;
        A.dualize();
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

TEST_CASE("Dualize by steps RUNCM") {
    Runc::RuncGlobalState A(Runc::RUNCM);
    A.loadMatrix("data/15_35_12226.hg");
    auto count = 0;
    for (auto r = A.dualize(100); !r.empty(); r = A.dualize(100)) {
        count += r.size();

        for (auto& cover : r) {
            std::sort(cover.begin(), cover.end());
            CHECK(std::unique(cover.begin(), cover.end()) == cover.end());
        }
    }
    CHECK(count == 12226);
}

TEST_CASE("Dualize to buffer RUNCM") {
    Runc::RuncGlobalState A(Runc::RUNCM);
    A.loadMatrix("data/15_35_12226.hg");

    std::vector<uint32_t> buffer;
    buffer.resize(1000);

    auto count = 0;
    for (auto r = A.dualize(buffer.size(), buffer.data()); r > 0; r = A.dualize(buffer.size(), buffer.data())) {
        count += r;
    }
    CHECK(count == 12226);
}
