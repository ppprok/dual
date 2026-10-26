// Weight-bound pruning callbacks (WeightBoundsPruningCallback, CoveredRowsWeighter) on RUNC-M, and
// with the completion bounds (#53) on every search-tree algorithm: the covers that survive pruning
// are compared with the full enumeration filtered by weight.
#include <doctest.h>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <dual/aot1.h>
#include <dual/cover_collector.h>
#include <dual/pruning.h>
#include <dual/punc.h>
#include <dual/runc.h>
#include <dual/weight_bounds_pruning.h>

using namespace dual;

namespace {

typedef std::vector<int> Cover;
// Cover (sorted columns) -> weight reported by the callback
typedef std::map<Cover, Weight> WeightedCoverMap;

// Runs RUNC-M on the file with the pruning callback installed as InnerCallback and, chained
// before a CoverCollector, as CoverCallback. Returns the covers that were not pruned.
template<typename Pruner>
WeightedCoverMap RunPruned(std::string const& file, Pruner& pruner) {
    struct Chain : IDualizationCallback {
        Pruner* pruner;
        CoverCollector collector;
        void Call(DualizationNode& node) override {
            pruner->Call(node);
            if (!node.stoped)
                collector.Call(node);
        }
    } chain;
    chain.pruner = &pruner;

    Runc::RuncGlobalState runc(Runc::RUNCM);
    runc.loadMatrix(file);
    runc.InnerCallback = &pruner;
    runc.CoverCallback = &chain;
    runc.dualize();

    WeightedCoverMap covers;
    for (auto const& c : chain.collector.GetCovers()) {
        Cover cover = c.columns;
        std::sort(cover.begin(), cover.end());
        CHECK_MESSAGE(covers.emplace(cover, c.weight).second, "repeated cover");
    }
    return covers;
}

std::vector<Cover> AllCovers(std::string const& file) {
    CoverCollector collector;
    Runc::RuncGlobalState runc(Runc::RUNCM);
    runc.loadMatrix(file);
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

// Integer column weights, some negative, so that sums are exact in float
Weight ColumnWeight(int j) {
    return (Weight) (j % 5 - 2);
}

}  // namespace

TEST_CASE("WeightBoundsPruning: TargetToMin and TargetToMax keep all lightest / heaviest covers") {
    char const* files[] = {"data/15_20_275.hg", "data/10_25_480.hg", "data/10_50_7187.hg"};
    for (auto file : files) {
        CAPTURE(std::string(file));
        auto all = AllCovers(file);
        size_t minSize = all.front().size(), maxSize = all.front().size();
        for (auto const& c : all) {
            minSize = std::min(minSize, c.size());
            maxSize = std::max(maxSize, c.size());
        }

        for (bool toMin : {true, false}) {
            CAPTURE(toMin);
            // Every column weighs 1, so the weight of a cover is its size
            WeightBoundsPruningCallback pruner;
            pruner.SetDefaultWeight(1);
            if (toMin)
                pruner.TargetToMin();
            else
                pruner.TargetToMax();

            auto found = RunPruned(file, pruner);
            CHECK(found.size() < all.size());

            size_t target = toMin ? minSize : maxSize;
            for (auto const& c : found)
                CHECK_EQ(c.second, (Weight) c.first.size());
            for (auto const& c : all)
                if (c.size() == target)
                    CHECK_MESSAGE(found.count(c), "a cover of the extreme size was pruned");
        }
    }
}

TEST_CASE("WeightBoundsPruning: fixed bounds keep exactly the covers with weight inside them") {
    char const* files[] = {"data/6_6_4.hg", "data/15_20_275.hg", "data/10_25_480.hg", "data/15_35_12226.hg"};
    for (auto file : files) {
        CAPTURE(std::string(file));
        auto all = AllCovers(file);

        // Explicit weights for the first 12 columns, the default weight 1 for the rest
        Weights weights;
        for (int j = 0; j < 12; ++j)
            weights.push_back(ColumnWeight(j));
        auto weightOf = [&](Cover const& c) {
            Weight w = 0;
            for (int j : c)
                w += j < (int) weights.size() ? weights[j] : 1;
            return w;
        };

        WeightBoundsPruningCallback pruner;
        pruner.SetWeights(weights);
        pruner.SetDefaultWeight(1);
        pruner.FixMinBound(-1);
        pruner.FixMaxBound(2);

        auto found = RunPruned(file, pruner);
        size_t expected = 0;
        for (auto const& c : all) {
            Weight w = weightOf(c);
            bool inside = w >= -1 && w <= 2;
            expected += inside;
            CHECK_EQ(inside, found.count(c) == 1);
        }
        CHECK_EQ(expected, found.size());
        for (auto const& c : found)
            CHECK_EQ(c.second, weightOf(c.first));
    }
}

TEST_CASE("CoveredRowsWeighter weighs covers by the target rows they hit and prunes by bounds") {
    char const* files[] = {"data/6_6_4.hg", "data/15_20_275.hg", "data/10_25_480.hg"};
    for (auto file : files) {
        CAPTURE(std::string(file));
        auto all = AllCovers(file);
        Runc::RuncGlobalState reader;
        reader.loadMatrix(file);
        int n = reader.get_matrix().width();

        // Target rows: row i holds the columns j with (i + j) % 3 == 0 or j == i; row weights 1..m
        int m = 7;
        bit_matrix target;  // the weighter keeps views of its rows, so it must outlive the run
        target.zero(m, n);
        Weights rowWeights;
        for (int i = 0; i < m; ++i) {
            rowWeights.push_back((Weight) (i + 1));
            for (int j = 0; j < n; ++j)
                if ((i + j) % 3 == 0 || j == i)
                    target.set(i, j);
        }
        auto weightOf = [&](Cover const& c) {
            Weight w = 0;
            for (int i = 0; i < m; ++i) {
                bool hit = false;
                for (int j : c)
                    hit |= ((i + j) % 3 == 0 || j == i);
                if (hit)
                    w += rowWeights[i];
            }
            return w;
        };

        for (auto bounds : {std::make_pair(0.0f, 1000.0f),
                            std::make_pair(0.0f, 20.0f),
                            std::make_pair(0.0f, 12.0f),
                            std::make_pair(24.0f, 1000.0f),
                            std::make_pair(15.0f, 22.0f)})
            for (bool completion : {false, true}) {
                CAPTURE(bounds.first);
                CAPTURE(bounds.second);
                CAPTURE(completion);
                CoveredRowsWeighter weighter;
                CoveredRowsWeighter::Options options;
                options.TargetRows = target.get_rows();
                options.RowsWeights = rowWeights;
                weighter.SetOptions(options);
                weighter.FixMinBound(bounds.first);
                weighter.FixMaxBound(bounds.second);
                if (completion)
                    weighter.EnableCompletionBounds(reader.get_matrix());

                auto found = RunPruned(file, weighter);
                size_t expected = 0;
                for (auto const& c : all) {
                    bool inside = weightOf(c) >= bounds.first && weightOf(c) <= bounds.second;
                    expected += inside;
                    CHECK_EQ(inside, found.count(c) == 1);
                }
                CHECK_EQ(expected, found.size());
                for (auto const& c : found)
                    CHECK_EQ(c.second, weightOf(c.first));
            }
    }
}

TEST_CASE("Completion bounds: fixed bounds on every search-tree algorithm and AOT1 variant") {
    // Rows {0,1}, {2,3}, {4,5}: every minimal cover has exactly 3 columns, one per row. With unit
    // weights and FixMinBound(4), the sums of the remaining weights never prune, the completion
    // bounds prune every node (a node with u uncovered rows reaches at most |S| + u = 3)
    bit_matrix three;
    three.zero(3, 6);
    for (int i = 0; i < 3; ++i) {
        three.set(i, 2 * i);
        three.set(i, 2 * i + 1);
    }

    // Data file with mixed-sign weights and both fixed bounds: exactly the covers inside them
    Runc::RuncGlobalState reader;
    reader.loadMatrix("data/15_20_275.hg");
    auto const& matrix = reader.get_matrix();
    auto all = AllCovers("data/15_20_275.hg");
    Weights weights;
    for (int j = 0; j < matrix.width(); ++j)
        weights.push_back(ColumnWeight(j) * 2 + 1);  // odd, -3..5
    auto weightOf = [&](Cover const& c) {
        Weight w = 0;
        for (int j : c)
            w += weights[j];
        return w;
    };

    std::vector<std::function<std::unique_ptr<DualizationBacktrackAlgorithmBase>()>> algorithms = {
            [] { return std::make_unique<Runc::RuncGlobalState>(Runc::RUNCM); },
            [] { return std::make_unique<Runc::RuncGlobalState>(Runc::RUNC); },
            [] { return std::make_unique<Runc::RuncGlobalState>(Runc::OPT); },
            [] { return std::make_unique<PUNC::GlobalState>(); }};
    for (auto type : {AOT1::AO1, AOT1::AO2, AOT1::AO1K, AOT1::AO2K, AOT1::AO1M, AOT1::AO2M})
        algorithms.push_back([type] { return std::make_unique<AOT1::GlobalState>(type); });

    for (size_t a = 0; a < algorithms.size(); ++a) {
        CAPTURE(a);
        for (bool completion : {false, true}) {
            CAPTURE(completion);
            long long nodes = 0;
            auto run = [&](bit_matrix const& m, WeightBoundsPruningCallback& pruner) {
                auto algorithm = algorithms[a]();
                bit_matrix copy;
                copy.zero(m.height(), m.width());
                for (int i = 0; i < m.height(); ++i)
                    copy.assign_row(i, m.get_row(i));
                algorithm->setMatrix(std::move(copy));
                if (completion)
                    pruner.EnableCompletionBounds(m);
                nodes = 0;
                std::set<Cover> found;
                auto inner = dualization_callback([&](DualizationNode& node) {
                    nodes += node.type == NodeType::NewNode;
                    pruner.Call(node);
                });
                auto cover = dualization_callback([&](DualizationNode& node) {
                    pruner.Call(node);
                    if (!node.IsCover())
                        return;
                    Cover c = node.columns;
                    std::sort(c.begin(), c.end());
                    CHECK_EQ(node.weight, weightOf(c));
                    CHECK(found.insert(c).second);
                });
                algorithm->InnerCallback = inner.get();
                algorithm->CoverCallback = cover.get();
                algorithm->dualize();
                return found;
            };

            WeightBoundsPruningCallback unit;
            unit.SetDefaultWeight(1);
            unit.FixMinBound(4);
            auto none = run(three, unit);
            CHECK(none.empty());
            if (completion)
                CHECK(nodes <= 6);  // only the first level, pruned at once
            else
                CHECK(nodes >= 8);  // at least the eight covers

            for (auto bounds :
                 {std::make_pair(-2.0f, 3.0f), std::make_pair(5.0f, 100.0f), std::make_pair(-100.0f, -2.0f)}) {
                CAPTURE(bounds.first);
                WeightBoundsPruningCallback pruner;
                pruner.SetWeights(weights);
                pruner.FixMinBound(bounds.first);
                pruner.FixMaxBound(bounds.second);
                auto found = run(matrix, pruner);
                std::set<Cover> expected;
                for (auto const& c : all)
                    if (weightOf(c) >= bounds.first && weightOf(c) <= bounds.second)
                        expected.insert(c);
                CHECK(found == expected);
            }
        }
    }
}
