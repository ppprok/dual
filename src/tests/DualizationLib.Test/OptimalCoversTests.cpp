// Optimal minimal covers through enumeration with branch pruning (issue #49): for every algorithm,
// functional and direction, the covers found with pruning are compared with the full enumeration
// filtered by weight (and that with brute force on small matrices). Includes the regression test for
// the floating-point drift of node weights and, skipped by default, the pruning measurements:
//     cd build/release/src/tests && ./TestDual -tc="OptimalCovers: measurements" --no-skip

#include <doctest.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <functional>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include <dual/aot1.h>
#include <dual/cover_collector.h>
#include <dual/optimal_covers.h>
#include <dual/pruning.h>
#include <dual/punc.h>
#include <dual/runc.h>
#include <dual/runc_fixed.h>
#include <dual/weight_bounds_pruning.h>
#include <dual/weighters.h>

#include "RuncTestUtils.h"

using namespace dual;
using namespace runc_test;

namespace {

CoverAlgorithm const allAlgorithms[] = {CoverAlgorithm::RUNCM,
                                        CoverAlgorithm::RUNC,
                                        CoverAlgorithm::OPT,
                                        CoverAlgorithm::AOT1,
                                        CoverAlgorithm::PUNC,
                                        CoverAlgorithm::FK};

char const* algorithmName(CoverAlgorithm a) {
    switch (a) {
        case CoverAlgorithm::RUNCM:
            return "RUNCM";
        case CoverAlgorithm::RUNC:
            return "RUNC";
        case CoverAlgorithm::OPT:
            return "OPT";
        case CoverAlgorithm::AOT1:
            return "AOT1";
        case CoverAlgorithm::PUNC:
            return "PUNC";
        case CoverAlgorithm::FK:
            return "FK";
    }
    return "?";
}

bit_matrix copyMatrix(bit_matrix const& matrix) {
    bit_matrix copy;
    copy.zero(matrix.height(), matrix.width());
    for (int i = 0; i < matrix.height(); ++i)
        copy.assign_row(i, matrix.get_row(i));
    return copy;
}

std::vector<Cover> allCovers(bit_matrix const& matrix) {
    CoverCollector collector;
    Runc::RuncGlobalState runc(Runc::RUNCM);
    runc.setMatrix(copyMatrix(matrix));
    runc.CoverCallback = &collector;
    runc.dualize();
    std::vector<Cover> covers;
    for (auto const& c : collector.GetCovers()) {
        Cover cover = c.columns;
        std::sort(cover.begin(), cover.end());
        covers.push_back(cover);
    }
    return covers;
}

std::vector<Cover> bruteForce(Rows const& rows) {
    int n = widthOf(rows);
    std::vector<Cover> result;
    for (unsigned s = 0; s < (1u << n); ++s) {
        Cover c;
        for (int j = 0; j < n; ++j)
            if (s >> j & 1u)
                c.push_back(j);
        if (isMinimalCover(rows, c))
            result.push_back(c);
    }
    std::sort(result.begin(), result.end());
    return result;
}

// Weight of a cover under a functional; computed in double, exact for the integer weights used here
typedef std::function<double(Cover const&)> CoverWeight;

// The covers of the optimal weight (within tolerance), sorted
std::set<Cover> filterOptimal(std::vector<Cover> const& covers,
                              CoverWeight const& weight,
                              Objective objective,
                              double tolerance = 0) {
    std::set<Cover> result;
    if (covers.empty())
        return result;
    double best = weight(covers.front());
    for (auto const& c : covers)
        best = objective == Objective::Minimize ? std::min(best, weight(c)) : std::max(best, weight(c));
    for (auto const& c : covers) {
        double w = weight(c);
        if (objective == Objective::Minimize ? w <= best + tolerance : w >= best - tolerance)
            result.insert(c);
    }
    return result;
}

std::set<Cover> coverSet(OptimalCovers const& result) {
    std::set<Cover> covers;
    for (auto const& c : result.covers)
        CHECK_MESSAGE(covers.insert(c.columns).second, "repeated cover");
    return covers;
}

// Column weights for a functional: empty (length), non-negative or mixed-sign integers
Weights columnWeights(std::mt19937& rng, int n, int kind) {
    Weights w;
    if (kind == 0)
        return w;
    for (int j = 0; j < n; ++j)
        w.push_back(kind == 1 ? (Weight) (rng() % 5) : (Weight) ((int) (rng() % 7) - 3));
    return w;
}

CoverWeight columnWeightOf(Weights const& weights) {
    return [weights](Cover const& c) {
        double w = 0;
        for (int j : c)
            w += j < (int) weights.size() ? weights[j] : 1;
        return w;
    };
}

CoverWeight coveredRowsWeightOf(Rows const& target, Weights const& weights) {
    return [target, weights](Cover const& c) {
        double w = 0;
        for (size_t i = 0; i < target.size(); ++i) {
            bool hit = false;
            for (int j : target[i])
                hit |= std::find(c.begin(), c.end(), j) != c.end();
            if (hit)
                w += i < weights.size() ? weights[i] : 1;
        }
        return w;
    };
}

Rows randomRows(std::mt19937& rng, int n, int m, int percent = 35) {
    Rows rows(m);
    for (auto& r : rows) {
        for (int j = 0; j < n; ++j)
            if ((int) (rng() % 100) < percent)
                r.push_back(j);
        if (r.empty())
            r.push_back((int) (rng() % n));
    }
    return rows;
}

Rows readRows(std::string const& file) {
    Runc::RuncGlobalState reader;
    reader.loadMatrix(file);
    auto const& bm = reader.get_matrix();
    Rows rows(bm.height());
    for (int i = 0; i < bm.height(); ++i) {
        auto const& r = bm.get_row(i);
        for (int j = r.find_first(); j >= 0; j = r.find_next(j))
            rows[i].push_back(j);
    }
    return rows;
}

// Matrix of exactly n columns (makeMatrix takes the width from the largest index)
bit_matrix makeMatrixOfWidth(Rows const& rows, int n) {
    bit_matrix bm;
    bm.zero((int) rows.size(), n);
    for (size_t i = 0; i < rows.size(); ++i)
        for (int j : rows[i])
            bm.set((int) i, j);
    return bm;
}

struct Stats {
    long long runs = 0;
    long long withTies = 0;
    long long nodesPruned = 0;
    long long nodesFull = 0;
};

// Checks every algorithm, both directions, with and without pruning against the filtered covers
void checkAllAlgorithms(std::vector<Cover> const& covers,
                        CoverWeight const& weight,
                        std::function<OptimalCovers(OptimalCoversOptions const&)> const& solve,
                        bool withFK,
                        Stats& stats) {
    for (auto objective : {Objective::Minimize, Objective::Maximize}) {
        CAPTURE((objective == Objective::Minimize ? "min" : "max"));
        auto expected = filterOptimal(covers, weight, objective);
        ++stats.runs;
        stats.withTies += expected.size() > 1;
        for (auto algorithm : allAlgorithms) {
            if (algorithm == CoverAlgorithm::FK && !withFK)
                continue;
            CAPTURE(algorithmName(algorithm));
            OptimalCoversOptions options;
            options.objective = objective;
            options.algorithm = algorithm;
            auto pruned = solve(options);
            options.prune = false;
            auto full = solve(options);

            CHECK(coverSet(pruned) == expected);
            CHECK(coverSet(full) == expected);
            if (!expected.empty()) {
                CHECK_EQ(pruned.weight, (Weight) weight(*expected.begin()));
                for (auto const& c : pruned.covers)
                    CHECK_EQ(c.weight, (Weight) weight(c.columns));
            }
            CHECK_EQ(full.coversVisited, (long long) covers.size());
            CHECK(pruned.nodes <= full.nodes);
            CHECK(pruned.coversVisited <= full.coversVisited);
            stats.nodesPruned += pruned.nodes;
            stats.nodesFull += full.nodes;
        }
    }
}

void checkColumnWeights(Rows const& rows, int n, std::mt19937& rng, bool withFK, Stats& stats) {
    auto matrix = makeMatrixOfWidth(rows, n);
    auto covers = allCovers(matrix);
    for (int kind : {0, 1, 2}) {
        CAPTURE(kind);
        auto weights = columnWeights(rng, n, kind);
        checkAllAlgorithms(
                covers,
                columnWeightOf(weights),
                [&](OptimalCoversOptions const& o) { return optimalCovers(matrix, weights, o); },
                withFK,
                stats);
    }
}

void checkCoveredRows(Rows const& rows, int n, std::mt19937& rng, bool withFK, Stats& stats) {
    auto matrix = makeMatrixOfWidth(rows, n);
    auto covers = allCovers(matrix);
    int m = 1 + (int) (rng() % 8);
    Rows target = randomRows(rng, n, m, 20);
    for (bool mixedSign : {false, true}) {
        CAPTURE(mixedSign);
        Weights weights;
        for (int i = 0; i < m; ++i)
            weights.push_back(mixedSign ? (Weight) ((int) (rng() % 7) - 3) : (Weight) (rng() % 4));
        auto targetMatrix = makeMatrixOfWidth(target, n);
        checkAllAlgorithms(
                covers,
                coveredRowsWeightOf(target, weights),
                [&](OptimalCoversOptions const& o) {
                    return optimalCoversByCoveredRows(matrix, targetMatrix, weights, o);
                },
                withFK,
                stats);
    }
}

}  // namespace

TEST_CASE("OptimalCovers: the full enumeration agrees with brute force on small random matrices") {
    std::mt19937 rng(49);
    for (int iter = 0; iter < 200; ++iter) {
        int n = 1 + (int) (rng() % 11);
        int m = 1 + (int) (rng() % 10);
        auto rows = randomRows(rng, n, m);
        CAPTURE(iter);
        auto runcm = allCovers(makeMatrixOfWidth(rows, n));
        std::sort(runcm.begin(), runcm.end());
        CHECK(runcm == bruteForce(rows));
    }
}

TEST_CASE("OptimalCovers: column weights, every algorithm, random matrices") {
    std::mt19937 rng(4901);
    Stats stats;
    for (int iter = 0; iter < 60; ++iter) {
        int n = 2 + (int) (rng() % 14);
        int m = 1 + (int) (rng() % 12);
        CAPTURE(iter);
        checkColumnWeights(randomRows(rng, n, m), n, rng, true, stats);
    }
    // Ties are common with small integer weights, so the "all optima, not one" part is exercised
    CHECK(stats.withTies * 10 > stats.runs);
    CHECK(stats.nodesPruned < stats.nodesFull);
}

TEST_CASE("OptimalCovers: covered rows weight, every algorithm, random matrices") {
    std::mt19937 rng(4902);
    Stats stats;
    for (int iter = 0; iter < 60; ++iter) {
        int n = 2 + (int) (rng() % 14);
        int m = 1 + (int) (rng() % 12);
        CAPTURE(iter);
        checkCoveredRows(randomRows(rng, n, m), n, rng, true, stats);
    }
    CHECK(stats.withTies * 10 > stats.runs);
    CHECK(stats.nodesPruned < stats.nodesFull);
}

TEST_CASE("OptimalCovers: data files, every algorithm") {
    // FK is slow on the larger files in Debug (see FKTests.cpp), so it is checked on the small ones
    char const* files[] = {"data/6_6_4.hg",
                           "data/6_6_6.hg",
                           "data/13_7_10.hg",
                           "data/15_10_14.hg",
                           "data/15_20_275.hg",
                           "data/10_25_480.hg",
                           "data/10_50_7187.hg"};
    std::mt19937 rng(4903);
    Stats stats;
    for (auto file : files) {
        CAPTURE(std::string(file));
        auto rows = readRows(file);
        int n = widthOf(rows);
        bool withFK = std::string(file) != "data/10_50_7187.hg";
        checkColumnWeights(rows, n, rng, withFK, stats);
        checkCoveredRows(rows, n, rng, withFK, stats);
    }
    CHECK(stats.nodesPruned < stats.nodesFull);
}

TEST_CASE("OptimalCovers: corner cases") {
    OptimalCoversOptions options;
    for (auto algorithm : allAlgorithms) {
        CAPTURE(algorithmName(algorithm));
        options.algorithm = algorithm;

        // No rows: the empty cover is the only one
        bit_matrix empty;
        empty.zero(0, 3);
        auto r = optimalCovers(empty, {}, options);
        REQUIRE_EQ(r.covers.size(), 1u);
        CHECK(r.covers[0].columns.empty());
        CHECK_EQ(r.weight, 0);

        // A zero row: no covers
        bit_matrix zero;
        zero.zero(2, 3);
        zero.set(0, 1);
        r = optimalCovers(zero, {}, options);
        CHECK(r.covers.empty());

        // Identity 3 x 3: one cover {0, 1, 2}
        bit_matrix identity;
        identity.zero(3, 3);
        for (int i = 0; i < 3; ++i)
            identity.set(i, i);
        r = optimalCovers(identity, {2, 5, 1}, options);
        REQUIRE_EQ(r.covers.size(), 1u);
        CHECK_EQ(r.weight, 8);
    }
}

TEST_CASE("OptimalCovers: tolerance keeps the near-optimal covers") {
    // Rows {0,1}, {2,3}: four covers {0,2}, {0,3}, {1,2}, {1,3}; weights 1, 2, 10, 10.5
    bit_matrix m = makeMatrix({{0, 1}, {2, 3}});
    Weights w = {1, 2, 10, 10.5f};
    for (auto algorithm : allAlgorithms) {
        CAPTURE(algorithmName(algorithm));
        OptimalCoversOptions options;
        options.algorithm = algorithm;
        auto r = optimalCovers(m, w, options);
        CHECK(coverSet(r) == std::set<Cover>{{0, 2}});
        options.tolerance = 1;
        r = optimalCovers(m, w, options);
        CHECK(coverSet(r) == std::set<Cover>{{0, 2}, {0, 3}, {1, 2}});
        CHECK_EQ(r.weight, 11);
        options.objective = Objective::Maximize;
        r = optimalCovers(m, w, options);
        CHECK(coverSet(r) == std::set<Cover>{{1, 3}, {0, 3}, {1, 2}});
    }
}

TEST_CASE("OptimalCoverCollector keeps ties and drops covers beaten later") {
    OptimalCoverCollector collector(Objective::Minimize);
    DualizationNode node;
    auto report = [&](Columns c, Weight w) {
        node.Reset();
        node.columns = std::move(c);
        node.weight = w;
        node.type = NodeType::CoverNode;
        collector.Call(node);
    };
    report({0}, 5);
    report({1}, 3);
    report({2}, 3);
    report({3}, 4);
    report({4}, 3);
    REQUIRE_EQ(collector.GetCovers().size(), 3u);
    CHECK_EQ(collector.GetBestWeight(), 3);
    CHECK_EQ(collector.GetCovers()[2].columns, Columns{4});

    // A pruned cover is ignored
    node.Reset();
    node.columns = {5};
    node.weight = 1;
    node.type = NodeType::CoverNode;
    node.pruned = true;
    collector.Call(node);
    CHECK_EQ(collector.GetCovers().size(), 3u);

    collector.Reset();
    CHECK_FALSE(collector.HasCovers());
}

TEST_CASE("OptimalCoverCollector with the existing weighters (WeighterBase) on every search-tree algorithm") {
    // ColumnsWeighterCallback / CoveredRowWeighterCallback weigh the nodes; the collector moves their
    // bound to the best weight, so they prune; all optimal covers must survive.
    std::mt19937 rng(4904);
    char const* files[] = {"data/15_20_275.hg", "data/10_25_480.hg"};
    for (auto file : files) {
        CAPTURE(std::string(file));
        auto rows = readRows(file);
        int n = widthOf(rows);
        auto matrix = makeMatrixOfWidth(rows, n);
        auto covers = allCovers(matrix);

        Weights colWeights;
        for (int j = 0; j < n; ++j)
            colWeights.push_back((Weight) (1 + rng() % 4));
        int m = 6;
        Rows target = randomRows(rng, n, m, 20);
        auto targetMatrix = makeMatrixOfWidth(target, n);
        Weights rowWeights;
        for (int i = 0; i < m; ++i)
            rowWeights.push_back((Weight) (rng() % 4));

        for (bool rowsWeighter : {false, true}) {
            CAPTURE(rowsWeighter);
            CoverWeight weight = rowsWeighter ? coveredRowsWeightOf(target, rowWeights) : columnWeightOf(colWeights);
            for (auto objective : {Objective::Minimize, Objective::Maximize}) {
                CAPTURE((objective == Objective::Minimize ? "min" : "max"));
                auto expected = filterOptimal(covers, weight, objective);
                for (int a = 0; a < 5; ++a) {
                    CAPTURE(a);
                    std::unique_ptr<DualizationBacktrackAlgorithmBase> alg;
                    if (a < 3)
                        alg = std::make_unique<Runc::RuncGlobalState>(a == 0 ? Runc::RUNCM
                                                                             : (a == 1 ? Runc::RUNC : Runc::OPT));
                    else if (a == 3)
                        alg = std::make_unique<AOT1::GlobalState>();
                    else
                        alg = std::make_unique<PUNC::GlobalState>();
                    alg->setMatrix(copyMatrix(matrix));

                    ColumnsWeighterCallback columnsWeighter;
                    columnsWeighter.SetWeights(colWeights);
                    CoveredRowWeighterCallback rowWeighter;
                    rowWeighter.SetTargetRows(targetMatrix.get_rows());
                    rowWeighter.SetWeights(rowWeights);
                    WeighterBase* weighter = rowsWeighter ? (WeighterBase*) &rowWeighter : &columnsWeighter;

                    OptimalCoverCollector collector(objective);
                    collector.Weighter = weighter;
                    alg->InnerCallback = weighter;
                    alg->CoverCallback = &collector;
                    alg->dualize();

                    std::set<Cover> found;
                    for (auto const& c : collector.GetCovers()) {
                        Cover s = c.columns;
                        std::sort(s.begin(), s.end());
                        found.insert(s);
                        CHECK_EQ(c.weight, (Weight) weight(s));
                    }
                    CHECK(found == expected);
                }
            }
        }
    }
}

TEST_CASE("OptimalCovers: RuncFixed has no callbacks, so optimal covers come from filtering") {
    std::mt19937 rng(4905);
    for (auto file : {"data/15_20_275.hg", "data/10_25_480.hg", "data/10_50_7187.hg"}) {
        CAPTURE(std::string(file));
        auto rows = readRows(file);
        int n = widthOf(rows);
        RuncFixed<bits64>::Rows input;
        for (auto const& r : rows) {
            bits64 b = 0;
            for (int j : r)
                b |= bits64(1) << j;
            input.push_back(b);
        }
        std::vector<Cover> covers;
        for (bits64 c : RuncFixedDualize<bits64>(input)) {
            Cover cover;
            for (int j = 0; j < 64; ++j)
                if (c >> j & 1u)
                    cover.push_back(j);
            covers.push_back(cover);
        }
        auto weights = columnWeights(rng, n, 1);
        auto matrix = makeMatrixOfWidth(rows, n);
        for (auto objective : {Objective::Minimize, Objective::Maximize}) {
            OptimalCoversOptions options;
            options.objective = objective;
            CHECK(filterOptimal(covers, columnWeightOf(weights), objective)
                  == coverSet(optimalCovers(matrix, weights, options)));
        }
    }
}

// Regression (#49): weighters updated node.weight by adding and subtracting float weights, so the
// weights drifted during the enumeration and a dynamic bound could prune tied optimal covers.
TEST_CASE("Weighters restore node weights exactly on backtrack (no floating-point drift)") {
    std::mt19937 rng(4906);
    for (auto file : {"data/10_50_7187.hg", "data/15_35_12226.hg"}) {
        CAPTURE(std::string(file));
        auto rows = readRows(file);
        int n = widthOf(rows);
        auto matrix = makeMatrixOfWidth(rows, n);
        Weights w;
        for (int j = 0; j < n; ++j)
            w.push_back((Weight) (1 + rng() % 9) * 0.1f);

        // Weight of a cover summed in the order its columns were added, as the weighters do
        auto pathSum = [&w](Columns const& c) {
            Weight s = 0;
            for (int j : c)
                s += w[j];
            return s;
        };

        WeightBoundsPruningCallback pruner;
        pruner.SetWeights(w);
        ColumnsWeighterCallback weighter;
        weighter.SetWeights(w);
        for (IDualizationCallback* inner : {(IDualizationCallback*) &pruner, (IDualizationCallback*) &weighter}) {
            Runc::RuncGlobalState runc(Runc::RUNCM);
            runc.setMatrix(copyMatrix(matrix));
            long long covers = 0, drifted = 0;
            auto check = dualization_callback([&](DualizationNode& node) {
                ++covers;
                drifted += node.weight != pathSum(node.columns);
            });
            runc.InnerCallback = inner;
            runc.CoverCallback = check.get();
            runc.dualize();
            CHECK(covers > 7000);
            CHECK_EQ(drifted, 0);
        }

        // With a tolerance, every cover within it of the optimum is found, whatever the summation order
        auto all = allCovers(matrix);
        for (auto objective : {Objective::Minimize, Objective::Maximize}) {
            OptimalCoversOptions options;
            options.objective = objective;
            options.tolerance = 1e-4f;
            CHECK(coverSet(optimalCovers(matrix, w, options))
                  == filterOptimal(all, columnWeightOf(w), objective, 1e-3));
        }
    }
}

TEST_CASE("OptimalCovers: measurements" * doctest::skip()) {
    struct Case {
        std::string name;
        Rows rows;
    };
    std::vector<Case> cases;
    for (auto file : {"data/15_20_275.hg", "data/10_50_7187.hg", "data/15_35_12226.hg"})
        cases.push_back({file, readRows(file)});
    std::mt19937 rng(4907);
    cases.push_back({"random 30x40 p=0.2", randomRows(rng, 40, 30, 20)});
    cases.push_back({"random 20x60 p=0.1", randomRows(rng, 60, 20, 10)});

    std::printf("%-22s %-14s %-4s %-6s %10s %10s %9s %9s %8s %8s\n",
                "input",
                "functional",
                "dir",
                "alg",
                "nodes",
                "nodes-full",
                "ms",
                "ms-full",
                "covers",
                "optimal");
    for (auto const& c : cases) {
        int n = widthOf(c.rows);
        auto matrix = makeMatrixOfWidth(c.rows, n);
        Weights weights;
        for (int j = 0; j < n; ++j)
            weights.push_back((Weight) (1 + rng() % 9));
        int m = 10;
        auto target = makeMatrixOfWidth(randomRows(rng, n, m, 10), n);
        Weights rowWeights;
        for (int i = 0; i < m; ++i)
            rowWeights.push_back((Weight) (1 + rng() % 9));

        for (int functional = 0; functional < 3; ++functional) {
            char const* fname = functional == 0 ? "length" : (functional == 1 ? "column-weight" : "covered-rows");
            for (auto objective : {Objective::Minimize, Objective::Maximize}) {
                for (auto algorithm : allAlgorithms) {
                    if (algorithm == CoverAlgorithm::FK)
                        continue;  // no search tree: the same work with and without pruning
                    OptimalCoversOptions options;
                    options.objective = objective;
                    options.algorithm = algorithm;
                    auto solve = [&]() {
                        if (functional == 0)
                            return optimalCovers(matrix, {}, options);
                        if (functional == 1)
                            return optimalCovers(matrix, weights, options);
                        return optimalCoversByCoveredRows(matrix, target, rowWeights, options);
                    };
                    auto t0 = std::chrono::steady_clock::now();
                    auto pruned = solve();
                    auto t1 = std::chrono::steady_clock::now();
                    options.prune = false;
                    auto full = solve();
                    auto t2 = std::chrono::steady_clock::now();
                    CHECK(coverSet(pruned) == coverSet(full));
                    std::printf("%-22s %-14s %-4s %-6s %10lld %10lld %9.2f %9.2f %8lld %8zu\n",
                                c.name.c_str(),
                                fname,
                                objective == Objective::Minimize ? "min" : "max",
                                algorithmName(algorithm),
                                pruned.nodes,
                                full.nodes,
                                std::chrono::duration<double, std::milli>(t1 - t0).count(),
                                std::chrono::duration<double, std::milli>(t2 - t1).count(),
                                full.coversVisited,
                                pruned.covers.size());
                }
            }
        }
    }
}
