#include <doctest.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <dual/aot1.h>
#include <dual/cover_collector.h>
#include <dual/punc.h>
#include <dual/runc.h>

#include "DualTestUtils.h"

using namespace dual;

namespace {

using namespace dual_test;

Covers RuncmCovers(bit_matrix matrix, int maxCount = -1) {
    Runc::RuncGlobalState runc(Runc::RUNCM);
    runc.setMatrix(std::move(matrix));
    return collect(runc, maxCount);
}

struct NamedAlgorithm {
    std::string name;
    std::function<std::unique_ptr<DualizationBacktrackAlgorithmBase>()> create;
};

std::vector<NamedAlgorithm> Algorithms() {
    std::vector<NamedAlgorithm> algs;
    std::pair<AOT1::AlgorithmType, const char*> types[] = {{AOT1::AO1, "AO1"},
                                                           {AOT1::AO2, "AO2"},
                                                           {AOT1::AO1K, "AO1K"},
                                                           {AOT1::AO2K, "AO2K"},
                                                           {AOT1::AO1M, "AO1M"},
                                                           {AOT1::AO2M, "AO2M"}};
    for (auto t : types) {
        auto type = t.first;
        algs.push_back({std::string("AOT1::") + t.second, [type] {
                            return std::unique_ptr<DualizationBacktrackAlgorithmBase>(new AOT1::GlobalState(type));
                        }});
    }
    algs.push_back(
            {"PUNC", [] { return std::unique_ptr<DualizationBacktrackAlgorithmBase>(new PUNC::GlobalState()); }});
    return algs;
}

}  // namespace

TEST_CASE("AOT1 and PUNC match RUNCM and brute force on random small matrices") {
    std::mt19937 random(23);
    auto algs = Algorithms();

    for (int it = 0; it < 600; ++it) {
        int n = 1 + random() % 12;
        int m = 1 + random() % 12;
        auto rows = randomRows(random, m, n);
        CAPTURE(it);

        auto expected = RuncmCovers(makeMatrix(rows, n));
        REQUIRE(expected == bruteForce(rows, n));

        for (auto const& alg : algs) {
            CAPTURE(alg.name);
            auto a = alg.create();
            a->setMatrix(makeMatrix(rows, n));
            CHECK(collect(*a) == expected);
        }
    }
}

TEST_CASE("AOT1 and PUNC match RUNCM on random larger matrices") {
    // Matrices with too many covers are skipped to keep the test fast
    const int maxCovers = 3000;
    std::mt19937 random(2323);
    auto algs = Algorithms();

    int tested = 0;
    for (int it = 0; it < 200 && tested < 60; ++it) {
        int n = 1 + random() % 40;
        int m = 1 + random() % 16;
        auto rows = randomRows(random, m, n);
        CAPTURE(it);

        auto expected = RuncmCovers(makeMatrix(rows, n), maxCovers + 1);
        if ((int) expected.size() > maxCovers)
            continue;
        ++tested;
        for (auto const& alg : algs) {
            CAPTURE(alg.name);
            auto a = alg.create();
            a->setMatrix(makeMatrix(rows, n));
            CHECK(collect(*a) == expected);
        }
    }
    CHECK(tested >= 30);
}

TEST_CASE("AOT1 and PUNC edge matrices") {
    // A single cover of the maximal depth: PUNC used to reallocate its states during recursion here
    Rows single = {{0}};
    Rows tall = {{0, 1}, {1, 2}, {0, 2}, {0}, {2}};
    Rows diagonal = {{0}, {1}, {2}, {3}, {4}, {5}};
    // A zero row with n < m: the only case where AOT1 reaches state q + 1, one more than it used to reserve
    Rows zeroRowTall = {{0}, {}, {0}, {0}};

    for (auto const& rows : {single, tall, diagonal, zeroRowTall}) {
        auto expected = RuncmCovers(makeMatrix(rows));
        if (rows == zeroRowTall)
            CHECK(expected.empty());
        for (auto const& alg : Algorithms()) {
            CAPTURE(alg.name);
            auto a = alg.create();
            a->setMatrix(makeMatrix(rows));
            CHECK(collect(*a) == expected);
        }
    }
}

TEST_CASE("All algorithms agree on the empty matrix") {
    // The dual of the empty hypergraph is {empty set}
    Covers expected = {Cover()};
    for (auto type : {Runc::OPT, Runc::RUNC, Runc::RUNCM}) {
        CAPTURE(type);
        Runc::RuncGlobalState runc(type);
        runc.setMatrix(bit_matrix());
        CHECK(collect(runc) == expected);
    }
    {
        // The resumable API reports the empty cover once, then nothing
        Runc::RuncGlobalState runc;
        runc.setMatrix(bit_matrix());
        CHECK_EQ(1, (int) runc.dualize(10).size());
        CHECK(runc.dualize(10).empty());
    }
    for (auto const& alg : Algorithms()) {
        CAPTURE(alg.name);
        auto a = alg.create();
        a->setMatrix(bit_matrix());
        CHECK(collect(*a) == expected);
    }
}

TEST_CASE("AOT1 and PUNC repeated dualize on the same object") {
    Rows small = {{0, 1}, {1, 2}};
    Rows big = loadRows("data/15_20_275.hg");

    auto expectedSmall = RuncmCovers(makeMatrix(small));
    auto expectedBig = RuncmCovers(makeMatrix(big));
    REQUIRE_EQ(2, (int) expectedSmall.size());
    REQUIRE_EQ(275, (int) expectedBig.size());

    for (auto const& alg : Algorithms()) {
        CAPTURE(alg.name);
        auto a = alg.create();

        a->setMatrix(makeMatrix(small));
        CHECK(collect(*a) == expectedSmall);
        CHECK(collect(*a) == expectedSmall);

        // A larger matrix on the same object, then back to the smaller one
        a->setMatrix(makeMatrix(big));
        CHECK(collect(*a) == expectedBig);
        CHECK(collect(*a) == expectedBig);

        a->setMatrix(makeMatrix(small));
        CHECK(collect(*a) == expectedSmall);
    }
}

TEST_CASE("PUNC with rows and columns order") {
    PUNC::GlobalState punc;
    punc.loadMatrix("data/10_25_480.hg");
    auto expected = RuncmCovers(makeMatrix(toRows(punc.get_matrix())));
    REQUIRE_EQ(480, (int) expected.size());

    Weights cw(25);
    for (int j = 0; j < 25; ++j)
        cw[j] = std::sin((Weight) j);

    Weights rw(10);
    for (int i = 0; i < 10; ++i)
        rw[i] = std::cos((Weight) i);

    punc.SetRowsOrder(rw);
    punc.SetColsOrder(std::move(cw));

    CHECK(collect(punc) == expected);
}
