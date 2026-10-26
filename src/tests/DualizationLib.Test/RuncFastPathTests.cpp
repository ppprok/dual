// Fixed-width fast path of RUNC (#56): on matrices of at most RuncGlobalState::FastPathMaxWidth
// columns it must report exactly the search tree of the general path (every inner, backtrack and
// cover node with the same DualizationNode contents, in the same order), also under pruning and
// stopping callbacks, and through the resumable APIs.

#include <doctest.h>

#include <climits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "RuncTestUtils.h"

using namespace dual;

using namespace runc_test;

namespace {

const Runc::AlgorithmType algorithms[] = {Runc::OPT, Runc::RUNC, Runc::RUNCM};

const char* name(Runc::AlgorithmType type) {
    switch (type) {
        case Runc::OPT:
            return "OPT";
        case Runc::RUNC:
            return "RUNC";
        case Runc::RUNCM:
            return "RUNCM";
    }
    return "?";
}

// Interference with the search: prune some new nodes, stop at some node. Random wide matrices can
// have astronomically many covers, so every trace stops at node 20000 at the latest (the prefix
// of the search tree is compared then).
struct Interference {
    int pruneEvery = 0;  // prune new nodes whose node id is divisible by this (0: never)
    int stopAt = 20000;  // stop at the node with this id
};

// Every callback call as numbers: kind (0 inner, 1 cover), node type, last column, node id,
// branch id, then the columns of covers
class Trace : public IDualizationCallback {
public:
    Trace(int kind, Interference interference, std::vector<int>& out) :
            kind_(kind), interference_(interference), out_(out) {}

    void Call(DualizationNode& node) override {
        out_.push_back(kind_);
        out_.push_back(node.type);
        out_.push_back(node.lastColumn);
        out_.push_back(node.nodeID);
        out_.push_back(node.branchID);
        if (kind_ == 1) {
            out_.push_back(static_cast<int>(node.columns.size()));
            out_.insert(out_.end(), node.columns.begin(), node.columns.end());
        }
        if (node.type == NodeType::NewNode && interference_.pruneEvery > 0
            && node.nodeID % interference_.pruneEvery == 0)
            node.pruned = true;
        if (node.nodeID >= interference_.stopAt)
            node.stoped = true;
    }

private:
    int kind_;
    Interference interference_;
    std::vector<int>& out_;
};

std::string describe(Rows const& rows) {
    std::ostringstream out;
    for (auto const& r : rows) {
        for (int j : r)
            out << j << ' ';
        out << '\n';
    }
    return out.str();
}

// The trace of a full dualize() on the fast or the general path
std::vector<int> trace(Rows const& rows, int n, Runc::AlgorithmType type, bool fast, Interference interference = {}) {
    Runc::RuncGlobalState g(type);
    g.setFastPathEnabled(fast);
    g.setMatrix(makeMatrix(rows, n));
    std::vector<int> out;
    Trace inner(0, interference, out);
    Trace cover(1, interference, out);
    g.InnerCallback = &inner;
    g.CoverCallback = &cover;
    g.dualize();
    bool fastExpected = fast && !rows.empty() && n <= Runc::RuncGlobalState::FastPathMaxWidth;
    // A zero row finishes the enumeration before a path is chosen
    for (auto const& r : rows)
        fastExpected &= !r.empty();
    CHECK(g.isFastPathActive() == fastExpected);
    return out;
}

// Covers in the order of the resumable API (at most 20000 of them)
std::vector<Cover> portions(Rows const& rows, int n, Runc::AlgorithmType type, bool fast, size_t portion) {
    Runc::RuncGlobalState g(type);
    g.setFastPathEnabled(fast);
    g.setMatrix(makeMatrix(rows, n));
    std::vector<Cover> out;
    for (auto v = g.dualize(portion); !v.empty() && out.size() < 20000; v = g.dualize(portion))
        out.insert(out.end(), v.begin(), v.end());
    if (out.size() > 20000)
        out.resize(20000);
    return out;
}

// Same trace on both paths for every algorithm, with and without interference
void checkSameTree(Rows const& rows, int n) {
    for (auto type : algorithms) {
        INFO(name(type) << ", " << n << " columns, matrix\n" << describe(rows));
        auto general = trace(rows, n, type, false);
        CHECK(trace(rows, n, type, true) == general);
        Interference prune{3};
        CHECK(trace(rows, n, type, true, prune) == trace(rows, n, type, false, prune));
        Interference stop{0, 40};
        CHECK(trace(rows, n, type, true, stop) == trace(rows, n, type, false, stop));
    }
}

Rows randomMatrix(std::mt19937& rng, int m, int n) {
    std::uniform_real_distribution<double> uniform(0, 1);
    double p = 0.05 + 0.6 * uniform(rng);
    Rows rows(m);
    for (auto& r : rows) {
        for (int j = 0; j < n; ++j)
            if (uniform(rng) < p)
                r.push_back(j);
        if (r.empty())
            r.push_back(static_cast<int>(rng() % n));
    }
    return rows;
}

}  // namespace

TEST_CASE("RuncFastPath: same search tree as the general path on every data file") {
    for (auto const& file : dual_test::dataFiles()) {
        auto rows = dual_test::loadRows(file.string());
        INFO(file.string());
        for (auto type : algorithms) {
            INFO(name(type));
            Interference whole{0, INT_MAX};
            CHECK(trace(rows, 0, type, true, whole) == trace(rows, 0, type, false, whole));
        }
    }
}

TEST_CASE("RuncFastPath: same search tree on random matrices around the word widths") {
    std::mt19937 rng(56);
    // Widths around 32 (one block of the general rows), 64 (one word) and 128 (two words)
    for (int n : {1, 2, 31, 32, 33, 63, 64, 65, 96, 127, 128, 129, 130}) {
        for (int it = 0; it < 4; ++it) {
            int m = 1 + static_cast<int>(rng() % 12);
            auto rows = randomMatrix(rng, m, n);
            checkSameTree(rows, n);
        }
    }
}

TEST_CASE("RuncFastPath: same search tree on small random matrices") {
    std::mt19937 rng(5656);
    for (int it = 0; it < 300; ++it) {
        int n = 1 + static_cast<int>(rng() % 12);
        int m = 1 + static_cast<int>(rng() % 12);
        auto rows = randomMatrix(rng, m, n);
        checkSameTree(rows, n);
    }
}

TEST_CASE("RuncFastPath: the width of the matrix, not of its rows, selects the path") {
    // Narrow rows in a wide matrix: the general path; a matrix of exactly the limit: the fast path
    Rows rows = {{0, 1}, {1, 2}, {0, 2}};
    for (int n : {3, 64, 65, Runc::RuncGlobalState::FastPathMaxWidth, Runc::RuncGlobalState::FastPathMaxWidth + 1}) {
        INFO(n << " columns");
        for (auto type : algorithms)
            CHECK(trace(rows, n, type, true) == trace(rows, n, type, false));
    }
}

TEST_CASE("RuncFastPath: corner cases") {
    std::vector<Rows> inputs = {Rows(), Rows{{}}, Rows{{0}, {}}, Rows{{0}}, Rows{{0, 1, 2}}, Rows{{5}, {5}, {5}}};
    for (auto const& rows : inputs) {
        for (int n : {0, 1, 6, 64, 200}) {
            if (n < widthOf(rows))
                continue;
            for (auto type : algorithms) {
                INFO(name(type) << ", " << n << " columns, matrix\n" << describe(rows));
                CHECK(trace(rows, n, type, true) == trace(rows, n, type, false));
                CHECK(portions(rows, n, type, true, 1) == portions(rows, n, type, false, 1));
            }
        }
    }
}

TEST_CASE("RuncFastPath: resumable API in portions") {
    std::mt19937 rng(560);
    for (int n : {20, 64, 100}) {
        auto rows = randomMatrix(rng, 10, n);
        for (auto type : algorithms) {
            INFO(name(type) << ", " << n << " columns");
            auto general = portions(rows, n, type, false, 7);
            CHECK(!general.empty());
            CHECK(portions(rows, n, type, true, 7) == general);
            CHECK(portions(rows, n, type, true, 1) == general);
        }
    }
}

TEST_CASE("RuncFastPath: switch, restart on matrix change and exceptions") {
    auto rows = dual_test::loadRows("data/15_20_275.hg");
    Runc::RuncGlobalState g;
    CHECK(g.isFastPathEnabled());
    CHECK(!g.isFastPathActive());
    g.setMatrix(makeMatrix(rows));
    auto first = g.dualize(100);
    CHECK(first.size() == 100);
    CHECK(g.isFastPathActive());

    // Switching the path resets the enumeration; the general path gives the same covers in order
    g.setFastPathEnabled(false);
    CHECK(!g.isFastPathEnabled());
    CHECK(!g.isFastPathActive());
    CHECK(g.dualize(100) == first);
    CHECK(!g.isFastPathActive());
    g.setFastPathEnabled(true);

    // Widening the matrix in place restarts on the general path
    CHECK(g.dualize(10).size() == 10);
    g.mutable_matrix() = makeMatrix(rows, Runc::RuncGlobalState::FastPathMaxWidth + 1);
    CHECK(!g.isFastPathActive());
    size_t count = 0;
    for (auto v = g.dualize(64); !v.empty(); v = g.dualize(64))
        count += v.size();
    CHECK(count == 275);
    CHECK(!g.isFastPathActive());

    // Back to a narrow matrix: the fast path again
    g.setMatrix(makeMatrix(rows));
    CHECK(g.dualize(1).size() == 1);
    CHECK(g.isFastPathActive());

    // An exception from a callback resets the enumeration; the next portion starts from the first cover
    auto thrower = dualization_callback([](DualizationNode&) { throw std::runtime_error("stop"); });
    g.InnerCallback = thrower.get();
    CHECK_THROWS_AS(g.dualize(1000), std::runtime_error);
    CHECK(!g.isFastPathActive());
    g.InnerCallback = nullptr;
    CHECK(g.dualize(100) == first);
    CHECK(g.isFastPathActive());
}
