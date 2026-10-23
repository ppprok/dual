// Weight-bound pruning callbacks (WeightBoundsPruningCallback, CoveredRowsWeighter) on RUNC-M: the
// covers that survive pruning are compared with the full enumeration filtered by weight.
#include <doctest.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <dual/cover_collector.h>
#include <dual/pruning.h>
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

        for (Weight maxBound : {1000.0f, 20.0f, 12.0f}) {
            CAPTURE(maxBound);
            CoveredRowsWeighter weighter;
            CoveredRowsWeighter::Options options;
            options.TargetRows = target.get_rows();
            options.RowsWeights = rowWeights;
            weighter.SetOptions(options);
            weighter.FixMaxBound(maxBound);

            auto found = RunPruned(file, weighter);
            size_t expected = 0;
            for (auto const& c : all) {
                bool inside = weightOf(c) <= maxBound;
                expected += inside;
                CHECK_EQ(inside, found.count(c) == 1);
            }
            CHECK_EQ(expected, found.size());
            for (auto const& c : found)
                CHECK_EQ(c.second, weightOf(c.first));
        }
    }
}
