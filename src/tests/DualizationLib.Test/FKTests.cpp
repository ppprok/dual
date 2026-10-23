// Tests of Khachiyan's incremental dualization (issue #43): the Fredman–Khachiyan duality test
// FK::isDual on dual and non-dual pairs, and FK::GlobalState against the file names, RUNCM and
// brute force.

#include <doctest.h>

#include <algorithm>
#include <chrono>
#include <random>
#include <set>
#include <string>
#include <vector>

#include <dual/cover_collector.h>
#include <dual/fk.h>

#include "RuncTestUtils.h"

using namespace dual;

using namespace runc_test;

namespace {

std::set<Cover> collect(DualizationBacktrackAlgorithmBase& alg, int maxCount = -1) {
    CoverCollector collector;
    collector.MaxCount = maxCount;
    alg.CoverCallback = &collector;
    alg.dualize();
    alg.CoverCallback = nullptr;
    std::set<Cover> covers;
    for (auto const& c : collector.GetCovers()) {
        Cover cover = c.columns;
        std::sort(cover.begin(), cover.end());
        CHECK_MESSAGE(covers.insert(cover).second, "repeated cover");
    }
    return covers;
}

std::set<Cover> fkCovers(Rows const& rows) {
    FK::GlobalState fk;
    fk.setMatrix(makeMatrix(rows));
    auto covers = collect(fk);
    // FK reports the columns in increasing order
    auto const& last = fk.getLastCover();
    CHECK(std::is_sorted(last.begin(), last.end()));
    return covers;
}

std::set<Cover> runcmCovers(Rows const& rows) {
    Runc::RuncGlobalState runc(Runc::RUNCM);
    runc.setMatrix(makeMatrix(rows));
    return collect(runc);
}

std::set<Cover> bruteForce(Rows const& rows) {
    int n = widthOf(rows);
    std::set<Cover> result;
    for (unsigned s = 0; s < (1u << n); ++s) {
        Cover c;
        for (int j = 0; j < n; ++j)
            if (s >> j & 1)
                c.push_back(j);
        if (isMinimalCover(rows, c))
            result.insert(c);
    }
    return result;
}

Rows toRows(std::set<Cover> const& covers) {
    return Rows(covers.begin(), covers.end());
}

Rows loadRows(std::string const& file) {
    Runc::RuncGlobalState reader;
    reader.loadMatrix(file);
    Rows rows(reader.get_matrix().height());
    for (int i = 0; i < (int) rows.size(); ++i) {
        auto const& r = reader.get_matrix().get_row(i);
        for (int j = r.find_first(); j >= 0; j = r.find_next(j))
            rows[i].push_back(j);
    }
    return rows;
}

// Matrix with n columns (at least), so that empty rows and families over no column can be built
bit_matrix matrixOf(Rows const& rows, int n = 0) {
    n = std::max(n, widthOf(rows));
    bit_matrix bm;
    bm.zero((int) rows.size(), n);
    for (size_t i = 0; i < rows.size(); ++i)
        for (int j : rows[i])
            bm.get_row((int) i).set(j);
    return bm;
}

bool contains(Cover const& big, Cover const& small) {
    return std::includes(big.begin(), big.end(), small.begin(), small.end());
}

bool disjoint(Cover const& a, Cover const& b) {
    for (int j : a)
        if (std::find(b.begin(), b.end(), j) != b.end())
            return false;
    return true;
}

// Checks that the result of isDual(h, g) is a valid answer of the given kind
void checkWitness(Rows const& h, Rows const& g, FK::DualityResult const& r, FK::Duality expected) {
    REQUIRE(r.result == expected);
    Cover const& x = r.witness;
    CHECK(std::is_sorted(x.begin(), x.end()));
    bool transversal = std::all_of(h.begin(), h.end(), [&x](Cover const& e) { return !disjoint(e, x); });
    bool containsG = std::any_of(g.begin(), g.end(), [&x](Cover const& s) {
        Cover sorted = s;
        std::sort(sorted.begin(), sorted.end());
        return contains(x, sorted);
    });
    if (expected == FK::Duality::NewTransversal) {
        CHECK(transversal);
        CHECK_FALSE(containsG);
    } else if (expected == FK::Duality::NotTransversal) {
        CHECK_FALSE(transversal);
        CHECK(containsG);
    } else {
        CHECK(x.empty());
    }
}

// Random hypergraph without empty edges
Rows randomRows(std::mt19937& rng, int n, int m) {
    double p = 0.1 + (rng() % 50) / 100.0;
    std::uniform_real_distribution<> coin(0, 1);
    Rows rows(m);
    for (auto& r : rows) {
        for (int j = 0; j < n; ++j)
            if (coin(rng) < p)
                r.push_back(j);
        if (r.empty())
            r.push_back(rng() % n);
    }
    return rows;
}

double seconds(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

void checkDataFile(std::string const& file, int expected) {
    CAPTURE(file);
    auto rows = loadRows(file);
    auto t0 = std::chrono::steady_clock::now();
    auto fk = fkCovers(rows);
    double fkTime = seconds(t0);
    t0 = std::chrono::steady_clock::now();
    auto runcm = runcmCovers(rows);
    double runcmTime = seconds(t0);
    CHECK_EQ((int) fk.size(), expected);
    CHECK(fk == runcm);
    MESSAGE(file << ": FK " << fkTime << " s, RUNCM " << runcmTime << " s");
}

}  // namespace

TEST_CASE("DualizeFK 6_6_4") {
    checkDataFile("data/6_6_4.hg", 4);
}

TEST_CASE("DualizeFK 6_6_6") {
    checkDataFile("data/6_6_6.hg", 6);
}

TEST_CASE("DualizeFK 13_7_10") {
    checkDataFile("data/13_7_10.hg", 10);
}

TEST_CASE("DualizeFK 15_10_14") {
    checkDataFile("data/15_10_14.hg", 14);
}

TEST_CASE("DualizeFK 15_20_275") {
    checkDataFile("data/15_20_275.hg", 275);
}

TEST_CASE("DualizeFK 10_25_480") {
    checkDataFile("data/10_25_480.hg", 480);
}

// The two largest files take about 2 s and 10 s in Release (RUNCM: 0.1 s), several times longer in
// Debug, so they run only in optimized builds (release and asan presets)
#ifdef NDEBUG
constexpr bool debugBuild = false;
#else
constexpr bool debugBuild = true;
#endif

TEST_CASE("DualizeFK 10_50_7187" * doctest::skip(debugBuild)) {
    checkDataFile("data/10_50_7187.hg", 7187);
}

TEST_CASE("DualizeFK 15_35_12226" * doctest::skip(debugBuild)) {
    checkDataFile("data/15_35_12226.hg", 12226);
}

TEST_CASE("FK next() finds the covers one by one") {
    auto rows = loadRows("data/15_20_275.hg");
    FK::GlobalState fk;
    fk.setMatrix(makeMatrix(rows));
    CoverCollector collector;
    fk.CoverCallback = &collector;
    std::set<Cover> covers;
    while (fk.next()) {
        auto const& c = fk.getLastCover();
        CHECK(isMinimalCover(rows, c));
        CHECK_MESSAGE(covers.insert(c).second, "repeated cover");
        CHECK_EQ(fk.getCovers().height(), (int) covers.size());
    }
    CHECK(fk.isDualizeFinished());
    CHECK_FALSE(fk.next());
    CHECK_EQ(covers.size(), 275u);
    CHECK_EQ(collector.GetCovers().size(), 275u);
    CHECK(covers == runcmCovers(rows));

    // dualize() starts again from no covers
    CHECK(collect(fk) == covers);
    // so does a change of the matrix
    fk.setMatrix(makeMatrix({{0, 1}, {1, 2}}));
    CHECK_FALSE(fk.isDualizeFinished());
    CHECK_EQ(fk.getCovers().height(), 0);
    int count = 0;
    while (fk.next())
        ++count;
    CHECK_EQ(count, 2);
}

TEST_CASE("FK corner cases") {
    // No rows: one cover, the empty one
    FK::GlobalState fk;
    fk.setMatrix(bit_matrix(0, 5));
    auto covers = collect(fk);
    REQUIRE_EQ(covers.size(), 1u);
    CHECK(covers.begin()->empty());
    fk.setMatrix(bit_matrix());
    CHECK_EQ(collect(fk).size(), 1u);

    // A zero row: no covers
    fk.setMatrix(matrixOf({{0, 1}, {}, {2}}));
    CHECK(collect(fk).empty());
    fk.setMatrix(bit_matrix(2, 0));
    CHECK(collect(fk).empty());
    CHECK_FALSE(fk.next());

    // Duplicate and dependent rows
    CHECK(fkCovers({{0, 1}, {0, 1}, {0, 1, 2}, {2, 3}}) == std::set<Cover>{{0, 2}, {1, 2}, {0, 3}, {1, 3}});
    // One row, one column
    CHECK(fkCovers({{0}}) == std::set<Cover>{{0}});
    // Columns that are in no row are in no cover
    fk.setMatrix(matrixOf({{1}, {3}}, 6));
    CHECK(collect(fk) == std::set<Cover>{{1, 3}});

    // A callback that stops the enumeration finishes it
    fk.setMatrix(makeMatrix(loadRows("data/15_20_275.hg")));
    CHECK_EQ(collect(fk, 10).size(), 10u);
    CHECK(fk.isDualizeFinished());
    CHECK_FALSE(fk.next());
    CHECK_EQ(collect(fk).size(), 275u);
}

TEST_CASE("FK matches RUNCM and brute force on random matrices") {
    std::mt19937 rng(43);
    for (int iter = 0; iter < 400; ++iter) {
        int n = 1 + rng() % 12;
        int m = 1 + rng() % 12;
        auto rows = randomRows(rng, n, m);
        CAPTURE(iter);
        auto fk = fkCovers(rows);
        CHECK(fk == runcmCovers(rows));
        CHECK(fk == bruteForce(rows));
    }
    // Wider matrices; FK slows down superlinearly in the number of covers (about 4 s in Release for
    // 10000 covers of a 15 x 39 matrix), so matrices with many covers are skipped
    int tested = 0;
    for (int iter = 0; iter < 80; ++iter) {
        int n = 10 + rng() % 30;
        int m = 5 + rng() % 15;
        auto rows = randomRows(rng, n, m);
        CAPTURE(iter);
        auto expected = runcmCovers(rows);
        if (expected.size() > 2000)
            continue;
        ++tested;
        CHECK(fkCovers(rows) == expected);
    }
    CHECK(tested >= 40);
}

TEST_CASE("FK duality test on dual and non-dual pairs") {
    using FK::Duality;
    std::mt19937 rng(4343);
    for (int iter = 0; iter < 200; ++iter) {
        CAPTURE(iter);
        int n = 2 + rng() % 14;
        auto h = randomRows(rng, n, 1 + rng() % 10);
        auto tr = runcmCovers(h);
        Rows g = toRows(tr);

        // Dual both ways
        checkWitness(h, g, FK::isDual(matrixOf(h), matrixOf(g)), Duality::Dual);
        checkWitness(g, h, FK::isDual(matrixOf(g), matrixOf(h)), Duality::Dual);

        // A cover is missing: the witness is a transversal that contains no remaining cover
        Rows missing = g;
        missing.erase(missing.begin() + rng() % missing.size());
        checkWitness(h, missing, FK::isDual(matrixOf(h, n), matrixOf(missing, n)), Duality::NewTransversal);

        // A cover is replaced by a proper subset of it, which is not a transversal
        Rows smaller = g;
        auto& c = smaller[rng() % smaller.size()];
        if (c.size() > 1) {
            c.erase(c.begin() + rng() % c.size());
            checkWitness(h, smaller, FK::isDual(matrixOf(h, n), matrixOf(smaller, n)), Duality::NotTransversal);
        }

        // Non-minimal sets and duplicates on either side do not change the answer
        Rows extra = g;
        extra.push_back(g.front());
        Cover super = g.front();
        for (int j = 0; j < n; ++j)
            if (!std::count(super.begin(), super.end(), j)) {
                super.push_back(j);
                break;
            }
        std::sort(super.begin(), super.end());
        extra.push_back(super);
        checkWitness(h, extra, FK::isDual(matrixOf(h, n), matrixOf(extra, n)), Duality::Dual);
    }
}

TEST_CASE("FK duality test corner cases") {
    using FK::Duality;
    // Empty H is dual only to G = {{}}
    checkWitness({}, {}, FK::isDual(bit_matrix(), bit_matrix()), Duality::NewTransversal);
    CHECK(FK::isDual(bit_matrix(), bit_matrix()).witness.empty());
    checkWitness({}, {{}}, FK::isDual(bit_matrix(), bit_matrix(1, 0)), Duality::Dual);
    checkWitness({}, {{0}}, FK::isDual(bit_matrix(0, 3), matrixOf({{0}})), Duality::NewTransversal);
    // H with an empty edge is dual only to empty G
    checkWitness({{}}, {}, FK::isDual(bit_matrix(1, 0), bit_matrix()), Duality::Dual);
    checkWitness({{}, {1}}, {}, FK::isDual(matrixOf({{}, {1}}), bit_matrix()), Duality::Dual);
    checkWitness({{}}, {{0}}, FK::isDual(bit_matrix(1, 3), matrixOf({{0}})), Duality::NotTransversal);
    checkWitness({{}}, {{}}, FK::isDual(bit_matrix(1, 0), bit_matrix(1, 0)), Duality::NotTransversal);
    // Empty G and a non-trivial H
    checkWitness({{0, 1}, {2}}, {}, FK::isDual(matrixOf({{0, 1}, {2}}), bit_matrix()), Duality::NewTransversal);
    // G = {{}} and a non-empty H: {} is not a transversal
    checkWitness({{0}}, {{}}, FK::isDual(matrixOf({{0}}), bit_matrix(1, 1)), Duality::NotTransversal);
    // Singletons
    checkWitness({{0, 1, 2}},
                 {{0}, {1}, {2}},
                 FK::isDual(matrixOf({{0, 1, 2}}), matrixOf({{0}, {1}, {2}})),
                 Duality::Dual);
    checkWitness({{0, 1, 2}},
                 {{0}, {2}},
                 FK::isDual(matrixOf({{0, 1, 2}}), matrixOf({{0}, {2}})),
                 Duality::NewTransversal);
    checkWitness({{0, 1}},
                 {{0}, {1}, {2}},
                 FK::isDual(matrixOf({{0, 1}}), matrixOf({{0}, {1}, {2}})),
                 Duality::NotTransversal);
    // A column of G that is in no set of H
    checkWitness({{0, 1}, {1, 2}},
                 {{1}, {0, 2, 3}},
                 FK::isDual(matrixOf({{0, 1}, {1, 2}}), matrixOf({{1}, {0, 2, 3}})),
                 Duality::NewTransversal);
    // A column of H that is in no set of G
    checkWitness({{0, 1}, {1, 2, 3}},
                 {{1}, {0, 2}},
                 FK::isDual(matrixOf({{0, 1}, {1, 2, 3}}), matrixOf({{1}, {0, 2}})),
                 Duality::NewTransversal);
    // The matching and its dual
    Rows matching = {{0, 1}, {2, 3}, {4, 5}};
    Rows dual = toRows(runcmCovers(matching));
    REQUIRE_EQ(dual.size(), 8u);
    checkWitness(matching, dual, FK::isDual(matrixOf(matching), matrixOf(dual)), Duality::Dual);
}

TEST_CASE("FK duality test on a data file") {
    auto h = loadRows("data/10_25_480.hg");
    Rows g = toRows(runcmCovers(h));
    checkWitness(h, g, FK::isDual(matrixOf(h), matrixOf(g)), FK::Duality::Dual);
    for (size_t k : {size_t(0), g.size() / 2, g.size() - 1}) {
        Rows missing = g;
        missing.erase(missing.begin() + k);
        auto r = FK::isDual(matrixOf(h), matrixOf(missing));
        checkWitness(h, missing, r, FK::Duality::NewTransversal);
    }
}
