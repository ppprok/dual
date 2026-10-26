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
#include <dual/completion_bounds.h>
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
    long long nodesMaxPruned = 0;  // maximisation only
    long long nodesMaxFull = 0;
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
            options.completionBounds = false;
            auto pruned49 = solve(options);  // only the sums of the remaining weights (#49)
            options.prune = false;
            auto full = solve(options);

            CHECK(coverSet(pruned) == expected);
            CHECK(coverSet(pruned49) == expected);
            CHECK(coverSet(full) == expected);
            // The completion bounds only add pruning, and pruned branches hold no cover that would
            // move the dynamic bound, so the nodes visited are a subset. Not for PUNC: it breaks ties
            // between columns of equal weight at random, and the random engine advances once per
            // expanded node, so pruning changes the order of the later branches.
            if (algorithm != CoverAlgorithm::PUNC)
                CHECK(pruned.nodes <= pruned49.nodes);
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
            if (objective == Objective::Maximize) {
                stats.nodesMaxPruned += pruned.nodes;
                stats.nodesMaxFull += full.nodes;
            }
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
    CHECK(stats.nodesMaxPruned < stats.nodesMaxFull);
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
    CHECK(stats.nodesMaxPruned < stats.nodesMaxFull);
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
    CHECK(stats.nodesMaxPruned * 2 < stats.nodesMaxFull);
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

// The completion bounds (#53) against brute force: for every column set S of a small random matrix,
// MaxGain and MinGain bound f(C) - f(S) over all minimal covers C that contain S. This is the
// algorithm-independent part of the admissibility argument (docs/optimal-covers.md).
TEST_CASE("CompletionBounds: the bounds hold for every column set on small random matrices") {
    std::mt19937 rng(5301);
    long long sets = 0, withCovers = 0, tightMax = 0, tightMin = 0;
    for (int iter = 0; iter < 150; ++iter) {
        int n = 1 + (int) (rng() % 9);
        int m = 1 + (int) (rng() % 8);
        auto rows = randomRows(rng, n, m, 30 + (int) (rng() % 30));
        auto matrix = makeMatrixOfWidth(rows, n);
        auto covers = bruteForce(rows);
        CAPTURE(iter);

        // Additive: column weights of both signs; not additive: weights of hit target rows
        Weights w;
        for (int j = 0; j < n; ++j)
            w.push_back((Weight) ((int) (rng() % 9) - 4));
        int t = 1 + (int) (rng() % 5);
        Rows target = randomRows(rng, n, t, 30);
        Weights tw;
        for (int i = 0; i < t; ++i)
            tw.push_back((Weight) ((int) (rng() % 9) - 4));

        for (bool additive : {true, false}) {
            CAPTURE(additive);
            Weights positive(n, 0), negative(n, 0);
            CoverWeight f;
            if (additive) {
                for (int j = 0; j < n; ++j)
                    (w[j] > 0 ? positive : negative)[j] = w[j];
                f = columnWeightOf(w);
            } else {
                for (int i = 0; i < t; ++i)
                    for (int j : target[i])
                        (tw[i] > 0 ? positive : negative)[j] += tw[i];
                f = coveredRowsWeightOf(target, tw);
            }

            CompletionBounds bounds;
            bounds.SetMatrix(matrix);
            bounds.SetGains(positive, negative, additive, 100, n);
            REQUIRE(bounds.Enabled());
            for (unsigned mask = 0; mask < (1u << n); ++mask) {
                Cover set;
                for (int j = 0; j < n; ++j)
                    if (mask >> j & 1u)
                        set.push_back(j);
                std::shuffle(set.begin(), set.end(), rng);

                // Walk down to S; the same object is reused, so states of earlier paths are stale
                DualizationNode node;
                for (int j : set) {
                    node.AddColumn(j);
                    bounds.Update(node);
                }
                std::sort(set.begin(), set.end());
                double fs = f(set);
                bool any = false;
                double best = 0, worst = 0;
                for (auto const& c : covers)
                    if (std::includes(c.begin(), c.end(), set.begin(), set.end())) {
                        double g = f(c) - fs;
                        best = any ? std::max(best, g) : g;
                        worst = any ? std::min(worst, g) : g;
                        any = true;
                    }
                ++sets;
                if (set.empty())
                    continue;  // the root is never a new node
                double maxGain = bounds.MaxGain();
                double minGain = bounds.MinGain();
                if (!any)
                    continue;
                ++withCovers;
                CAPTURE(mask);
                CHECK(maxGain >= best);
                CHECK(minGain <= worst);
                tightMax += maxGain == best;
                tightMin += minGain == worst;

                // Early stops keep the answer on the right side of the threshold
                for (double stop : {best - 1, best, best + 0.5, maxGain, maxGain + 1})
                    CHECK(bounds.MaxGainBelow(stop) == (maxGain < stop));
                for (double stop : {worst - 1, worst, worst - 0.5, minGain, minGain - 1})
                    CHECK(bounds.MinGainAbove(stop) == (minGain > stop));
            }
        }
    }
    CHECK(withCovers > 1000);
    // The bounds are not trivial: often exact
    CHECK(tightMax * 4 > withCovers);
    CHECK(tightMin * 4 > withCovers);
    MESSAGE("sets " << sets << ", with covers " << withCovers << ", tight max " << tightMax << ", tight min "
                    << tightMin);
}

// Cases that stress the completion bounds: many ties (length, weights 1 or 2), maximisation, both
// signs, and fractional weights with tolerance 0, where only the floating-point slack keeps pruned
// and full enumeration equal (the covers have the same path-order weights in both).
TEST_CASE("OptimalCovers: completion bounds with many ties and fractional weights") {
    std::mt19937 rng(5302);
    long long nodes = 0, nodes49 = 0, nodesFull = 0;
    for (int iter = 0; iter < 40; ++iter) {
        int n = 4 + (int) (rng() % 14);
        int m = 2 + (int) (rng() % 14);
        auto rows = randomRows(rng, n, m, 15 + (int) (rng() % 30));
        auto matrix = makeMatrixOfWidth(rows, n);
        CAPTURE(iter);
        for (int kind = 0; kind < 4; ++kind) {
            CAPTURE(kind);
            Weights w;
            for (int j = 0; j < n; ++j) {
                if (kind == 0)
                    w.push_back(1);  // the length
                else if (kind == 1)
                    w.push_back((Weight) (1 + rng() % 2));
                else if (kind == 2)
                    w.push_back((Weight) (1 + rng() % 9) * 0.1f);
                else
                    w.push_back((Weight) ((int) (rng() % 9) - 4) * 0.3f);
            }
            for (auto objective : {Objective::Minimize, Objective::Maximize}) {
                CAPTURE((objective == Objective::Minimize ? "min" : "max"));
                for (auto algorithm : allAlgorithms) {
                    if (algorithm == CoverAlgorithm::FK)
                        continue;
                    CAPTURE(algorithmName(algorithm));
                    OptimalCoversOptions options;
                    options.objective = objective;
                    options.algorithm = algorithm;
                    auto pruned = optimalCovers(matrix, w, options);
                    options.completionBounds = false;
                    auto pruned49 = optimalCovers(matrix, w, options);
                    options.prune = false;
                    auto full = optimalCovers(matrix, w, options);
                    CHECK(coverSet(pruned) == coverSet(full));
                    CHECK(coverSet(pruned49) == coverSet(full));
                    if (algorithm != CoverAlgorithm::PUNC)  // random tie-breaking, see checkAllAlgorithms
                        CHECK(pruned.nodes <= pruned49.nodes);
                    nodes += pruned.nodes;
                    nodes49 += pruned49.nodes;
                    nodesFull += full.nodes;
                }
            }
        }
    }
    CHECK(nodes * 4 < nodes49 * 3);
    MESSAGE("nodes " << nodes << ", without completion bounds " << nodes49 << ", full " << nodesFull);
}

TEST_CASE("CompletionBounds: corner cases") {
    CompletionBounds bounds;
    CHECK_FALSE(bounds.Enabled());
    CHECK_FALSE(bounds.HasMatrix());

    // No rows: every node is a cover, nothing can be gained
    bit_matrix empty;
    empty.zero(0, 3);
    bounds.SetMatrix(empty);
    CHECK(bounds.HasMatrix());
    CHECK_FALSE(bounds.Enabled());
    bounds.SetGains({1, 1, 1}, {}, true, 3, 3);
    CHECK(bounds.Enabled());
    DualizationNode node;
    node.AddColumn(1);
    bounds.Update(node);
    CHECK_EQ(bounds.MaxGain(), 0);
    CHECK_EQ(bounds.MinGain(), 0);

    // Rows {0, 1}, {0, 1, 2} (not minimal), {2, 3}: after {0} only {2, 3} is uncovered, so one more
    // column of weight 3 or 4 (or a column past the matrix, which hits nothing)
    bit_matrix m = makeMatrix({{0, 1}, {0, 1, 2}, {2, 3}});
    bounds.SetMatrix(m);
    bounds.SetGains({1, 2, 3, 4}, {}, true, 10, 4);
    node.Reset();
    node.AddColumn(0);
    bounds.Update(node);
    CHECK_EQ(bounds.MaxGain(), 4);
    CHECK_EQ(bounds.MinGain(), 3);
    node.AddColumn(7);
    bounds.Update(node);
    CHECK_EQ(bounds.MaxGain(), 4);
    node.Backtrack();
    node.Backtrack();
    node.AddColumn(2);  // hits {2, 3}; {0, 1} is left
    bounds.Update(node);
    CHECK_EQ(bounds.MaxGain(), 2);
    CHECK_EQ(bounds.MinGain(), 1);
    CHECK(bounds.Slack(0) > 0);
    CHECK(bounds.Slack(0) < 1e-4);

    bounds.Clear();
    CHECK_FALSE(bounds.Enabled());
}

TEST_CASE("OptimalCovers: measurements" * doctest::skip()) {
    // For every case: pruning with the completion bounds (#53, the default), pruning with the sums of
    // the remaining weights only (#49) and the full enumeration; times are the best of at least three
    // runs (more for short ones, up to 100 ms in total)
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

    std::printf("%-20s %-13s %-3s %-5s %9s %9s %9s %8s %8s %8s %8s %8s %6s\n",
                "input",
                "functional",
                "dir",
                "alg",
                "nodes",
                "nodes-49",
                "full",
                "ms",
                "ms-49",
                "ms-full",
                "ns/node",
                "ns/n-49",
                "optim");
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
                    auto solve = [&](bool prune, bool completion, double& ms) {
                        OptimalCoversOptions options;
                        options.objective = objective;
                        options.algorithm = algorithm;
                        options.prune = prune;
                        options.completionBounds = completion;
                        OptimalCovers result;
                        ms = 1e300;
                        double total = 0;
                        for (int run = 0; run < 3 || (total < 100 && run < 1000); ++run) {
                            auto t0 = std::chrono::steady_clock::now();
                            if (functional == 0)
                                result = optimalCovers(matrix, {}, options);
                            else if (functional == 1)
                                result = optimalCovers(matrix, weights, options);
                            else
                                result = optimalCoversByCoveredRows(matrix, target, rowWeights, options);
                            auto t1 = std::chrono::steady_clock::now();
                            double t = std::chrono::duration<double, std::milli>(t1 - t0).count();
                            ms = std::min(ms, t);
                            total += t;
                        }
                        return result;
                    };
                    double ms = 0, ms49 = 0, msFull = 0;
                    auto pruned = solve(true, true, ms);
                    auto pruned49 = solve(true, false, ms49);
                    auto full = solve(false, false, msFull);
                    CHECK(coverSet(pruned) == coverSet(full));
                    CHECK(coverSet(pruned49) == coverSet(full));
                    std::printf("%-20s %-13s %-3s %-5s %9lld %9lld %9lld %8.2f %8.2f %8.2f %8.0f %8.0f %6zu\n",
                                c.name.c_str(),
                                fname,
                                objective == Objective::Minimize ? "min" : "max",
                                algorithmName(algorithm),
                                pruned.nodes,
                                pruned49.nodes,
                                full.nodes,
                                ms,
                                ms49,
                                msFull,
                                ms * 1e6 / (double) std::max(pruned.nodes, 1LL),
                                ms49 * 1e6 / (double) std::max(pruned49.nodes, 1LL),
                                pruned.covers.size());
                }
            }
        }
    }
}
