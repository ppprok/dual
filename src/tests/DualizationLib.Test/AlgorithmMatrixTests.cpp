// Every dualization algorithm of the library with every option (#47): on every data file against
// the count in its name and RUNCM, on small random matrices against brute force, and the
// behaviour shared by the DualizationBacktrackAlgorithmBase algorithms (stop, callback exceptions,
// a consistent search tree for the InnerCallback).

#include <doctest.h>

#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <dual/aot1.h>
#include <dual/counters.h>
#include <dual/fk.h>
#include <dual/punc.h>
#include <dual/runc.h>
#include <dual/runc_fixed.h>

#include "DualTestUtils.h"

using namespace dual;
using namespace dual_test;

namespace {

typedef std::unique_ptr<DualizationBacktrackAlgorithmBase> AlgorithmPtr;

// An algorithm derived from DualizationBacktrackAlgorithmBase with one choice of its options
struct BacktrackAlgorithm {
    std::string name;
    std::function<AlgorithmPtr(int n)> create;  // n: number of columns, for the order weights
    bool slowOnLargeFiles = false;              // FK: see Enumerator::slowOnLargeFiles
};

// PUNC with a row and a column order given by weights; ties are broken at random inside PUNC
AlgorithmPtr puncWithOrder(int m, int n) {
    auto punc = std::make_unique<PUNC::GlobalState>();
    Weights rows(m), cols(n);
    for (int i = 0; i < m; ++i)
        rows[i] = std::cos((Weight) i);
    for (int j = 0; j < n; ++j)
        cols[j] = std::sin((Weight) j);
    punc->SetRowsOrder(rows);
    punc->SetColsOrder(cols);
    return punc;
}

std::vector<BacktrackAlgorithm> backtrackAlgorithms() {
    std::vector<BacktrackAlgorithm> algs;
    std::pair<Runc::AlgorithmType, const char*> runcTypes[] = {{Runc::OPT, "OPT"},
                                                               {Runc::RUNC, "RUNC"},
                                                               {Runc::RUNCM, "RUNCM"}};
    for (auto t : runcTypes) {
        auto type = t.first;
        algs.push_back({std::string("Runc::") + t.second,
                        [type](int) { return AlgorithmPtr(new Runc::RuncGlobalState(type)); }});
    }
    std::pair<AOT1::AlgorithmType, const char*> aotTypes[] = {{AOT1::AO1, "AO1"},
                                                              {AOT1::AO2, "AO2"},
                                                              {AOT1::AO1K, "AO1K"},
                                                              {AOT1::AO2K, "AO2K"},
                                                              {AOT1::AO1M, "AO1M"},
                                                              {AOT1::AO2M, "AO2M"}};
    for (auto t : aotTypes) {
        auto type = t.first;
        algs.push_back(
                {std::string("AOT1::") + t.second, [type](int) { return AlgorithmPtr(new AOT1::GlobalState(type)); }});
    }
    algs.push_back({"PUNC", [](int) { return AlgorithmPtr(new PUNC::GlobalState()); }});
    // The order weights are set before the matrix: SetRowsOrder must not depend on the matrix
    // that is current when it is called (#47)
    algs.push_back({"PUNC rows and columns order", [](int n) { return puncWithOrder(n + 3, n); }});
    algs.push_back({"FK", [](int) { return AlgorithmPtr(new FK::GlobalState()); }, true});
    return algs;
}

// Any way to get all minimal covers of a matrix, including the APIs that are not a plain dualize()
struct Enumerator {
    std::string name;
    std::function<Covers(Rows const& rows, int n)> run;
    int maxWidth = 1 << 30;  // RuncFixed: the width of its row type
    // Skipped on the data files with more than 5000 covers: FK takes seconds there in Release and
    // minutes in Debug, and next() restarts the duality test for every cover; FKTests.cpp runs
    // FK dualize() on those files in optimized builds
    bool slowOnLargeFiles = false;
};

Covers runcPortions(Rows const& rows, int n, Runc::AlgorithmType type, size_t portion) {
    Runc::RuncGlobalState runc(type);
    runc.setMatrix(makeMatrix(rows, n));
    Covers covers;
    for (auto v = runc.dualize(portion); !v.empty(); v = runc.dualize(portion))
        for (auto const& c : v)
            CHECK_MESSAGE(covers.insert(sorted(c)).second, "repeated cover");
    return covers;
}

Covers fkNext(Rows const& rows, int n) {
    FK::GlobalState fk;
    fk.setMatrix(makeMatrix(rows, n));
    Covers covers;
    while (fk.next())
        CHECK_MESSAGE(covers.insert(fk.getLastCover()).second, "repeated cover");
    return covers;
}

template<typename T>
Covers runcFixed(Rows const& rows) {
    Covers covers;
    for (auto mask : RuncFixedDualize<T>(toBits<T>(rows)))
        CHECK_MESSAGE(covers.insert(fromBits(mask)).second, "repeated cover");
    return covers;
}

template<typename T>
Covers runcFixedObject(Rows const& rows) {
    RuncFixed<T> runc;
    runc.callbackDelay = 7;
    int calls = 0;
    runc.callback = [&calls](RuncFixed<T>*) {
        ++calls;
        return true;
    };
    CHECK(runc.Dualize(toBits<T>(rows)));
    CHECK_EQ(runc.resultsCount, (T) runc.idealIndep.size());
    CHECK(runc.filterIndep.empty());
    // Every callbackDelay results and once after the last (incomplete) portion
    CHECK_EQ(calls, (int) (runc.idealIndep.size() + 6) / 7);
    Covers covers;
    for (auto mask : runc.idealIndep)
        CHECK_MESSAGE(covers.insert(fromBits(mask)).second, "repeated cover");
    return covers;
}

std::vector<Enumerator> enumerators() {
    std::vector<Enumerator> result;
    for (auto const& alg : backtrackAlgorithms()) {
        auto create = alg.create;
        result.push_back({alg.name + " dualize()",
                          [create](Rows const& rows, int n) {
                              auto a = create(n);
                              a->setMatrix(makeMatrix(rows, n));
                              return collect(*a);
                          },
                          1 << 30,
                          alg.slowOnLargeFiles});
    }
    result.push_back({"Runc::RUNCM dualize(max_count)",
                      [](Rows const& rows, int n) { return runcPortions(rows, n, Runc::RUNCM, 5); }});
    result.push_back({"Runc::OPT dualize(max_count)",
                      [](Rows const& rows, int n) { return runcPortions(rows, n, Runc::OPT, 64); }});
    result.push_back({"FK next()", fkNext, 1 << 30, true});
    result.push_back({"RuncFixedDualize<bits32>", [](Rows const& rows, int) { return runcFixed<bits32>(rows); }, 32});
    result.push_back({"RuncFixedDualize<bits64>", [](Rows const& rows, int) { return runcFixed<bits64>(rows); }, 64});
    result.push_back(
            {"RuncFixed<bits32> callback", [](Rows const& rows, int) { return runcFixedObject<bits32>(rows); }, 32});
    result.push_back(
            {"RuncFixed<bits64> callback", [](Rows const& rows, int) { return runcFixedObject<bits64>(rows); }, 64});
    return result;
}

Covers runcmCovers(Rows const& rows, int n = 0) {
    Runc::RuncGlobalState runc(Runc::RUNCM);
    runc.setMatrix(makeMatrix(rows, n));
    return collect(runc);
}

// Checks the search tree reported to the InnerCallback: a new node adds exactly lastColumn to the
// columns of its parent, a backtrack node removes it again, covers are reported at the current node
struct TreeChecker : IDualizationCallback {
    bool coverIsPath = true;  // false for FK, which reports covers without a search tree
    Columns path;
    int newNodes = 0;
    int errors = 0;
    int covers = 0;

    void Call(DualizationNode& node) override {
        switch (node.type) {
            case NodeType::NewNode:
                ++newNodes;
                path.push_back(node.lastColumn);
                if (node.columns != path)
                    ++errors;
                break;
            case NodeType::BacktrackNode:
                if (path.empty() || path.back() != node.lastColumn)
                    ++errors;
                else
                    path.pop_back();
                if (node.columns != path)
                    ++errors;
                break;
            case NodeType::CoverNode:
                // Only as CoverCallback: the cover is the current path
                ++covers;
                if (coverIsPath && node.columns != path)
                    ++errors;
                break;
            case NodeType::ExtraNode:
            case NodeType::RootNode:
                break;
        }
    }
};

struct ThrowingCallback : IDualizationCallback {
    int calls = 0;
    int throwAt = 0;
    void Call(DualizationNode& node) override {
        if (node.IsCover() && ++calls == throwAt)
            throw std::runtime_error("callback failure");
    }
};

}  // namespace

TEST_CASE("AlgorithmMatrix: every algorithm finds the covers of every data file") {
    auto files = dataFiles();
    REQUIRE(files.size() >= 8);
    auto all = enumerators();

    for (auto const& file : files) {
        CAPTURE(file.string());
        auto rows = loadRows(file.string());
        int n = widthOf(rows);
        auto expected = runcmCovers(rows);
        int count = expectedCount(file);
        if (count >= 0)
            CHECK_EQ(count, (int) expected.size());
        bool large = expected.size() > 5000;

        for (auto const& e : all) {
            if (n > e.maxWidth || (large && e.slowOnLargeFiles))
                continue;
            CAPTURE(e.name);
            CHECK(e.run(rows, n) == expected);
        }
    }
}

TEST_CASE("AlgorithmMatrix: every algorithm matches brute force on small random matrices") {
    std::mt19937 random(47);
    auto all = enumerators();

    // The matrix without rows has exactly the empty cover, also with columns
    for (int n : {0, 3}) {
        for (auto const& e : all) {
            CAPTURE(e.name);
            CAPTURE(n);
            CHECK(e.run(Rows(), n) == Covers{Cover()});
        }
    }
    // Rows over no column are zero rows: no covers
    for (auto const& e : all) {
        CAPTURE(e.name);
        CHECK(e.run(Rows(3), 0).empty());
    }

    for (int it = 0; it < 300; ++it) {
        int n = 1 + random() % 10;
        int m = 1 + random() % 10;
        auto rows = randomRows(random, m, n);
        CAPTURE(it);
        auto expected = bruteForce(rows, n);
        for (auto const& e : all) {
            CAPTURE(e.name);
            CHECK(e.run(rows, n) == expected);
        }
    }
}

TEST_CASE("AlgorithmMatrix: RuncFixed uses the highest column of its row type") {
    // Column 31 (bits32) and 63 (bits64) are the sign bits of a signed type of the same width
    Rows rows32 = {{0, 31}, {31, 5}, {5, 7}};
    Covers expected32 = {{0, 5}, {5, 31}, {7, 31}};
    CHECK(runcmCovers(rows32) == expected32);
    CHECK(runcFixed<bits32>(rows32) == expected32);
    CHECK(runcFixed<bits64>(rows32) == expected32);
    Rows rows64 = {{0, 63}, {63, 40}, {40, 2}, {1, 2, 62}};
    CHECK(runcFixed<bits64>(rows64) == runcmCovers(rows64));
}

TEST_CASE("AlgorithmMatrix: a CoverCallback stop ends the enumeration after that many covers") {
    auto rows = loadRows("data/15_20_275.hg");
    auto all = runcmCovers(rows);
    REQUIRE_EQ(275u, all.size());

    for (auto const& alg : backtrackAlgorithms()) {
        CAPTURE(alg.name);
        auto a = alg.create(widthOf(rows));
        a->setMatrix(makeMatrix(rows));
        for (int k : {1, 7, 274, 275}) {
            CAPTURE(k);
            auto some = collect(*a, k);
            CHECK_EQ(k, (int) some.size());
            for (auto const& c : some)
                CHECK(all.count(c) == 1);
        }
        // The stopped object enumerates everything again
        CHECK(collect(*a) == all);
    }
}

TEST_CASE("AlgorithmMatrix: the InnerCallback sees a consistent search tree") {
    std::mt19937 random(4747);
    std::vector<Rows> inputs = {loadRows("data/15_20_275.hg"), loadRows("data/13_7_10.hg"), Rows{{0}}, Rows()};
    for (int it = 0; it < 40; ++it)
        inputs.push_back(randomRows(random, 1 + random() % 9, 1 + random() % 9));

    for (auto const& alg : backtrackAlgorithms()) {
        CAPTURE(alg.name);
        for (auto const& rows : inputs) {
            auto a = alg.create(widthOf(rows));
            a->setMatrix(makeMatrix(rows));
            TreeChecker tree;
            tree.coverIsPath = alg.name != "FK";
            a->InnerCallback = &tree;
            // The same checker as CoverCallback: the cover is the path to the current node
            a->CoverCallback = &tree;
            a->dualize();
            CHECK_EQ(0, tree.errors);
            CHECK_EQ(runcmCovers(rows).size(), (size_t) tree.covers);
            // Every column added is removed again by the end of the enumeration
            CHECK(tree.path.empty());
        }
    }
}

TEST_CASE("AlgorithmMatrix: Counters count covers and inner nodes of every algorithm") {
    auto rows = loadRows("data/15_10_14.hg");
    for (auto const& alg : backtrackAlgorithms()) {
        CAPTURE(alg.name);
        auto a = alg.create(widthOf(rows));
        a->setMatrix(makeMatrix(rows));
        Counters counters;
        a->InnerCallback = &counters;
        a->CoverCallback = &counters;
        a->dualize();
        CHECK_EQ(14, counters.cover);
        CHECK(counters.length >= counters.cover);
        // FK has no backtracking search tree, so it reports no inner nodes
        if (alg.name == "FK")
            CHECK_EQ(0, counters.inner);
        else
            CHECK(counters.inner >= counters.cover);
        std::ostringstream out;
        auto* saved = std::cout.rdbuf(out.rdbuf());
        counters.Print();
        std::cout.rdbuf(saved);
        CHECK(out.str().find("count: 14\n") == 0);
        counters.Reset();
        CHECK_EQ(0, counters.cover);
        CHECK_EQ(0, counters.inner);
    }
}

TEST_CASE("AlgorithmMatrix: an exception from a callback leaves every algorithm reusable") {
    auto rows = loadRows("data/15_20_275.hg");
    auto all = runcmCovers(rows);

    for (auto const& alg : backtrackAlgorithms()) {
        CAPTURE(alg.name);
        auto a = alg.create(widthOf(rows));
        a->setMatrix(makeMatrix(rows));
        ThrowingCallback thrower;
        thrower.throwAt = 3;
        a->CoverCallback = &thrower;
        CHECK_THROWS_AS(a->dualize(), std::runtime_error);
        a->CoverCallback = nullptr;
        CHECK(collect(*a) == all);
    }

    // The batched RUNC APIs reset the enumeration when the InnerCallback throws
    for (auto type : {Runc::OPT, Runc::RUNC, Runc::RUNCM}) {
        CAPTURE(type);
        Runc::RuncGlobalState runc(type);
        runc.setMatrix(makeMatrix(rows));
        struct ThrowOnNewNode : IDualizationCallback {
            int calls = 0;
            void Call(DualizationNode& node) override {
                if (node.type == NodeType::NewNode && ++calls == 50)
                    throw std::logic_error("inner failure");
            }
        } inner;
        runc.InnerCallback = &inner;
        std::vector<uint32_t> buffer(1000);
        CHECK_THROWS_AS(runc.dualize((uint32_t) buffer.size(), buffer.data()), std::logic_error);
        CHECK_FALSE(runc.isDualizeFinished());
        runc.InnerCallback = nullptr;
        Covers covers;
        for (size_t c; (c = runc.dualize((uint32_t) buffer.size(), buffer.data())) > 0;) {
            auto* p = buffer.data();
            for (size_t k = 0; k < c; ++k) {
                covers.insert(sorted(Cover(p + 1, p + 1 + *p)));
                p += *p + 1;
            }
        }
        CHECK(covers == all);
        CHECK(runc.isDualizeFinished());
    }
}

TEST_CASE("AlgorithmMatrix: RuncFixed callback stops the enumeration") {
    auto rows = loadRows("data/15_20_275.hg");
    for (int delay : {1, 10, 300}) {
        CAPTURE(delay);
        RuncFixed<bits32> runc;
        runc.callbackDelay = delay;
        int calls = 0;
        runc.callback = [&calls](RuncFixed<bits32>* r) {
            ++calls;
            return r->idealIndep.size() < 100;
        };
        bool complete = runc.Dualize(toBits<bits32>(rows));
        // The callback sees at least 100 results first after ceil(100 / delay) calls
        if (delay < 275) {
            CHECK_FALSE(complete);
            CHECK_EQ(calls, (100 + delay - 1) / delay);
            CHECK_EQ(runc.idealIndep.size(), (size_t) (calls * delay));
        } else {
            // Called once, after the last result
            CHECK_FALSE(complete);
            CHECK_EQ(calls, 1);
            CHECK_EQ(runc.idealIndep.size(), 275u);
        }
        CHECK(runc.innerCount > 0);
    }
    // Without a callback the results are only collected
    RuncFixed<bits64> runc;
    CHECK(runc.Dualize(toBits<bits64>(rows)));
    CHECK_EQ(runc.resultsCount, 275u);
    CHECK(runc.innerCount > 0);
}

TEST_CASE("AlgorithmMatrix: RuncFixed DualizeIntervals finds disjoint pairs of minimal covers") {
    std::mt19937 random(470);
    for (int it = 0; it < 300; ++it) {
        int n = 1 + random() % 8;
        auto a = randomRows(random, 1 + random() % 6, n, 0);
        auto b = randomRows(random, 1 + random() % 6, n, 0);
        if (it % 50 == 0)
            b.clear();  // B without rows: its only minimal cover is the empty set
        CAPTURE(it);

        std::set<std::pair<Cover, Cover>> expected;
        for (auto const& x : bruteForce(a, n))
            for (auto const& y : bruteForce(b, n)) {
                Cover both;
                std::set_intersection(x.begin(), x.end(), y.begin(), y.end(), std::back_inserter(both));
                if (both.empty())
                    expected.insert({x, y});
            }

        auto check = [&](auto bits) {
            typedef decltype(bits) T;
            RuncFixed<T> runc;
            int calls = 0;
            runc.callbackDelay = 3;
            runc.callback = [&calls](RuncFixed<T>*) {
                ++calls;
                return true;
            };
            CHECK(runc.DualizeIntervals(toBits<T>(a), toBits<T>(b)));
            REQUIRE_EQ(runc.idealIndep.size(), runc.filterIndep.size());
            std::set<std::pair<Cover, Cover>> found;
            for (size_t k = 0; k < runc.idealIndep.size(); ++k)
                CHECK_MESSAGE(found.insert({fromBits(runc.idealIndep[k]), fromBits(runc.filterIndep[k])}).second,
                              "repeated pair");
            CHECK(found == expected);
            CHECK_EQ(calls, (int) (found.size() + 2) / 3);
            CHECK_EQ((size_t) RuncFixedDualize<T>(toBits<T>(a), toBits<T>(b)), expected.size());
        };
        check(bits32(0));
        check(bits64(0));
    }

    // A callback stop also ends DualizeIntervals
    RuncFixed<bits64> runc;
    runc.callbackDelay = 2;
    runc.callback = [](RuncFixed<bits64>*) { return false; };
    Rows a = {{0, 1}, {2, 3}}, b = {{4, 5}, {6, 7}};
    CHECK_FALSE(runc.DualizeIntervals(toBits<bits64>(a), toBits<bits64>(b)));
    CHECK_EQ(runc.idealIndep.size(), 2u);
}

// Regression (#47): SetRowsOrder built the order for the matrix current at the call, so a taller
// matrix set afterwards was traversed through an order with too few rows (read past its end), and
// weights set before the matrix were ignored
TEST_CASE("AlgorithmMatrix: PUNC row order applies to the matrix of dualize()") {
    struct FirstColumn : IDualizationCallback {
        int column = -1;
        void Call(DualizationNode& node) override {
            if (column < 0 && node.type == NodeType::NewNode)
                column = node.lastColumn;
        }
    };

    auto small = loadRows("data/10_25_480.hg");
    auto tall = loadRows("data/15_20_275.hg");
    PUNC::GlobalState punc;
    punc.setMatrix(makeMatrix(small));
    // Row 12 first: it exists only in the taller matrix
    Weights weights(15, 1);
    weights[12] = 0;
    punc.SetRowsOrder(weights);
    punc.setMatrix(makeMatrix(tall));
    FirstColumn first;
    punc.InnerCallback = &first;
    CHECK(collect(punc) == runcmCovers(tall));
    auto const& row = tall[12];
    CHECK(std::find(row.begin(), row.end(), first.column) != row.end());

    // Weights set before any matrix are used too; no weights restore the plain row order
    PUNC::GlobalState fresh;
    fresh.SetRowsOrder(weights);
    fresh.setMatrix(makeMatrix(tall));
    first.column = -1;
    fresh.InnerCallback = &first;
    CHECK(collect(fresh) == runcmCovers(tall));
    CHECK(std::find(row.begin(), row.end(), first.column) != row.end());
    fresh.SetRowsOrder(Weights());
    first.column = -1;
    CHECK(collect(fresh) == runcmCovers(tall));
    CHECK(std::find(tall[0].begin(), tall[0].end(), first.column) != tall[0].end());
}

TEST_CASE("AlgorithmMatrix: AOT1 local states are movable") {
    // GlobalState reserves its states so that they never move during the search; the move
    // operations are public, check that they keep the state usable
    AOT1::LocalState a;
    AOT1::LocalState b(std::move(a));
    auto* self = &b;
    b = std::move(*self);
    CHECK(b.IsCover());
    std::vector<AOT1::LocalState> states(3);
    states.reserve(states.capacity() + 1);
    CHECK(states.front().IsCover());
}
