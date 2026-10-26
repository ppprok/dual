// Differential test of RUNC (issue #24): random matrices, OPT/RUNC/RUNCM, the resumable C++ API and
// the C ABI with tiny buffers against each other and, for small widths, against brute force.

#include <doctest.h>

#include <random>
#include <sstream>
#include <string>

#include <dual/cover_collector.h>

#include "RuncTestUtils.h"

using namespace dual;

using namespace runc_test;

namespace {

// Mostly matrices without zero rows; rarely an empty matrix (exactly the empty cover) or a zero row (no covers)
Rows randomRows(std::mt19937& rng, int max_n, int max_m) {
    if (rng() % 50 == 0)
        return {};
    int n = 1 + rng() % max_n;
    int m = 1 + rng() % max_m;
    double p = 0.05 + (rng() % 50) / 100.0;
    std::uniform_real_distribution<> coin(0, 1);
    Rows rows(m);
    for (auto& r : rows) {
        if (rng() % 100 == 0)
            continue;
        for (int j = 0; j < n; ++j)
            if (coin(rng) < p)
                r.push_back(j);
        if (r.empty())
            r.push_back(rng() % n);
    }
    return rows;
}

std::string describe(Rows const& rows) {
    std::ostringstream out;
    for (auto const& r : rows) {
        for (int j : r)
            out << j << ' ';
        out << '\n';
    }
    return out.str();
}

CoverSet runFull(Rows const& rows, Runc::AlgorithmType type) {
    Runc::RuncGlobalState g(type);
    g.setMatrix(makeMatrix(rows));
    CoverCollector collector;
    g.CoverCallback = &collector;
    g.dualize();
    CoverSet s;
    for (auto const& c : collector.GetCovers())
        s.add(rows, c.columns);
    return s;
}

CoverSet runSteps(Rows const& rows, Runc::AlgorithmType type, size_t portion) {
    Runc::RuncGlobalState g(type);
    g.setMatrix(makeMatrix(rows));
    CoverSet s;
    for (auto v = g.dualize(portion); !v.empty(); v = g.dualize(portion))
        for (auto& c : v)
            s.add(rows, c);
    return s;
}

CoverSet runBuffer(Rows const& rows, Runc::AlgorithmType type, uint32_t extra) {
    Runc::RuncGlobalState g(type);
    g.setMatrix(makeMatrix(rows));
    std::vector<uint32_t> buffer(widthOf(rows) + 1 + extra);
    CoverSet s;
    for (size_t c; (c = g.dualize(uint32_t(buffer.size()), buffer.data())) > 0;)
        addRecords(rows, s, buffer.data(), c);
    return s;
}

void checkAll(Rows const& rows, std::set<Cover> const* reference) {
    int n = widthOf(rows);
    bool capiOk = true;
    std::vector<std::pair<std::string, CoverSet>> results = {
            {"OPT", runFull(rows, Runc::OPT)},
            {"RUNC", runFull(rows, Runc::RUNC)},
            {"RUNCM", runFull(rows, Runc::RUNCM)},
            {"RUNCM dualize(3)", runSteps(rows, Runc::RUNCM, 3)},
            {"OPT dualize(1)", runSteps(rows, Runc::OPT, 1)},
            {"RUNC buffer width+1", runBuffer(rows, Runc::RUNC, 0)},
            {"RUNCM buffer width+6", runBuffer(rows, Runc::RUNCM, 5)},
            {"C ABI buffer width+1", enumerateCApi(rows, n + 1, &capiOk)},
    };
    auto const& expected = reference ? *reference : results.front().second.covers;
    // No structured bindings: INFO captures `name` in a lambda, which is C++20 for a binding (clang warns)
    for (auto const& result : results) {
        auto const& name = result.first;
        auto const& r = result.second;
        INFO(name << " on matrix\n" << describe(rows));
        CHECK(r.duplicates == 0);
        CHECK(r.notMinimal == 0);
        CHECK(r.covers == expected);
    }
    INFO("C ABI status or canaries on matrix\n" << describe(rows));
    CHECK(capiOk);
}

}  // namespace

TEST_CASE("RuncDifferential: small matrices against brute force") {
    std::mt19937 rng(20261020);
    for (int it = 0; it < 1000; ++it) {
        auto rows = randomRows(rng, 10, 10);
        auto reference = dual_test::bruteForce(rows);
        checkAll(rows, &reference);
    }
}

TEST_CASE("RuncDifferential: larger matrices, algorithms and APIs agree") {
    std::mt19937 rng(24);
    for (int it = 0; it < 40; ++it) {
        auto rows = randomRows(rng, 40, 9);
        checkAll(rows, nullptr);
    }
}
