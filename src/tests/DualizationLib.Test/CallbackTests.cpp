// Callbacks and helpers around the algorithms (#47): the weighters of weighters.h on several
// algorithms against brute force filtered by weight, MostCoverCollector with a weighter,
// BitCoverCollectorCallback, the cover file writers, the counters, ChainCallback, restrictions,
// CoveredRowsWeighter targets and the dependent row removal of BoolMatrixPreparation.

#include <doctest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <dual/aot1.h>
#include <dual/counters.h>
#include <dual/cover_collector.h>
#include <dual/dualization_counters.h>
#include <dual/file_writers.h>
#include <dual/most_cover_collector.h>
#include <dual/pruning.h>
#include <dual/punc.h>
#include <dual/runc.h>
#include <dual/weighters.h>

#include "DualTestUtils.h"
#include "dual/bool_matrix_preparation.h"

using namespace dual;
using namespace dual_test;

namespace {

typedef std::unique_ptr<DualizationBacktrackAlgorithmBase> AlgorithmPtr;
typedef std::map<Cover, Weight> WeightedCoverMap;

// The algorithms that report a search tree to the InnerCallback, which the weighters need
std::vector<std::pair<std::string, std::function<AlgorithmPtr()>>> treeAlgorithms() {
    return {
            {"Runc::RUNCM", [] { return AlgorithmPtr(new Runc::RuncGlobalState(Runc::RUNCM)); }},
            {"Runc::OPT", [] { return AlgorithmPtr(new Runc::RuncGlobalState(Runc::OPT)); }},
            {"Runc::RUNC", [] { return AlgorithmPtr(new Runc::RuncGlobalState(Runc::RUNC)); }},
            {"AOT1::AO1", [] { return AlgorithmPtr(new AOT1::GlobalState(AOT1::AO1)); }},
            {"AOT1::AO2M", [] { return AlgorithmPtr(new AOT1::GlobalState(AOT1::AO2M)); }},
            {"PUNC", [] { return AlgorithmPtr(new PUNC::GlobalState()); }},
    };
}

// All minimal covers (RUNCM is checked against brute force in AlgorithmMatrixTests.cpp)
Covers allCovers(Rows const& rows) {
    Runc::RuncGlobalState runc(Runc::RUNCM);
    runc.setMatrix(makeMatrix(rows));
    return collect(runc);
}

// Runs the algorithm with the weighter as InnerCallback and a MostCoverCollector that keeps every
// cover inside the weighter's bounds; returns the kept covers with their weights
WeightedCoverMap runWeighted(DualizationBacktrackAlgorithmBase& alg, WeighterBase& weighter) {
    MostCoverCollector collector(&weighter);
    collector.Options = MostCoverCollectorOptions(1 << 20);  // no trimming
    alg.InnerCallback = &weighter;
    alg.CoverCallback = &collector;
    alg.dualize();
    alg.InnerCallback = nullptr;
    alg.CoverCallback = nullptr;

    WeightedCoverMap result;
    for (auto const& c : collector.GetCovers())
        CHECK_MESSAGE(result.emplace(sorted(c.columns), c.weight).second, "repeated cover");
    return result;
}

// Checks that exactly the covers of `all` with weight in [lo, hi] were kept, with their weights
void checkKept(Covers const& all,
               WeightedCoverMap const& kept,
               std::function<Weight(Cover const&)> const& weightOf,
               Weight lo,
               Weight hi) {
    size_t expected = 0;
    for (auto const& c : all) {
        Weight w = weightOf(c);
        bool inside = w >= lo && w <= hi;
        expected += inside;
        CHECK_MESSAGE(inside == (kept.count(c) == 1), "weight " << w);
    }
    CHECK_EQ(expected, kept.size());
    for (auto const& c : kept)
        CHECK_EQ(c.second, weightOf(c.first));
}

const Weight inf = std::numeric_limits<Weight>::max();

std::vector<std::string> readLines(std::string const& file) {
    std::ifstream in(file);
    std::vector<std::string> lines;
    for (std::string line; std::getline(in, line);)
        lines.push_back(line);
    return lines;
}

// A cover file writer: the callback and its file
struct Writer {
    std::shared_ptr<IDualizationCallback> callback;
    IntegersFileWriter* file;
};

template<typename T>
Writer makeWriter() {
    auto writer = std::make_shared<T>();
    return {writer, writer.get()};
}

// Columns j of the row with j % step == 0
Cover maskedColumns(bit_chunk const& row, int step) {
    Cover c;
    for (int j = row.find_first(); j >= 0; j = row.find_next(j))
        if (j % step == 0)
            c.push_back(j);
    return c;
}

std::string join(Cover const& c) {
    std::ostringstream out;
    for (size_t k = 0; k < c.size(); ++k)
        out << (k ? " " : "") << c[k];
    return out.str();
}

}  // namespace

TEST_CASE("Weighters: ColumnsWeighterCallback keeps the covers with weight inside the bounds") {
    std::mt19937 random(4701);
    std::vector<Rows> inputs = {loadRows("data/15_20_275.hg"), loadRows("data/10_25_480.hg")};
    for (int it = 0; it < 30; ++it)
        inputs.push_back(randomRows(random, 1 + random() % 8, 1 + random() % 10, 0));

    // Integer weights (exact float sums); the last weight is the default for the other columns
    std::vector<Weights> weightSets = {{1}, {2, 0, 1, 3}, {-1, 2, -2, 1, 0}, {1, -1, 2, -1}, {3, 1, 2, 0}};
    struct Bounds {
        Weight lo, hi;
    } bounds[] = {{-inf, inf}, {-inf, 3}, {2, inf}, {0, 2}, {-1, 1}};

    for (auto const& alg : treeAlgorithms()) {
        CAPTURE(alg.first);
        for (size_t k = 0; k < inputs.size(); ++k) {
            auto const& rows = inputs[k];
            auto all = allCovers(rows);
            for (size_t w = 0; w < weightSets.size(); ++w) {
                auto const& weights = weightSets[w];
                CAPTURE(w);
                auto weightOf = [&weights](Cover const& c) {
                    Weight w = 0;
                    for (int j : c)
                        w += j < (int) weights.size() ? weights[j] : weights.back();
                    return w;
                };
                for (auto b : bounds) {
                    CAPTURE(k);
                    CAPTURE(b.lo);
                    CAPTURE(b.hi);
                    ColumnsWeighterCallback weighter;
                    weighter.SetWeights(weights);
                    weighter.SetMinWeight(b.lo);
                    weighter.SetMaxWeight(b.hi);
                    auto a = alg.second();
                    a->setMatrix(makeMatrix(rows));
                    checkKept(all, runWeighted(*a, weighter), weightOf, b.lo, b.hi);
                }
            }
        }
    }
}

TEST_CASE("Weighters: CoveredRowWeighterCallback weighs covers by the target rows they hit") {
    std::mt19937 random(4702);
    for (int it = 0; it < 40; ++it) {
        int n = 1 + random() % 10;
        auto rows = it == 0 ? loadRows("data/15_20_275.hg") : randomRows(random, 1 + random() % 8, n, 0);
        n = widthOf(rows);
        auto all = allCovers(rows);
        // Random target rows over the same columns, integer row weights (the last is the default)
        auto targets = randomRows(random, 1 + random() % 6, n, 3);
        bit_matrix target = makeMatrix(targets, n);  // the weighter keeps views of its rows
        Weights weights;
        for (size_t i = 0; i + 1 < targets.size(); ++i)
            weights.push_back((Weight) (int(random() % 5) - 1));
        weights.push_back(1);
        auto weightOf = [&](Cover const& c) {
            Weight w = 0;
            for (size_t i = 0; i < targets.size(); ++i) {
                bool hit = false;
                for (int j : c)
                    hit |= std::find(targets[i].begin(), targets[i].end(), j) != targets[i].end();
                if (hit)
                    w += i < weights.size() ? weights[i] : weights.back();
            }
            return w;
        };

        for (auto const& alg : treeAlgorithms()) {
            CAPTURE(alg.first);
            CAPTURE(it);
            for (Weight lo : {-inf, 0.0f, 1.0f})
                for (Weight hi : {inf, 1.0f, 2.0f}) {
                    CoveredRowWeighterCallback weighter;
                    weighter.SetTargetRows(target.get_rows());
                    weighter.SetWeights(weights);
                    weighter.SetMinWeight(lo);
                    weighter.SetMaxWeight(hi);
                    auto a = alg.second();
                    a->setMatrix(makeMatrix(rows));
                    checkKept(all, runWeighted(*a, weighter), weightOf, lo, hi);
                }
        }
    }
}

TEST_CASE("Weighters: the base class helpers") {
    ColumnsWeighterCallback weighter;
    CHECK_EQ(0, weighter.GetDefaultWeight());
    CHECK_EQ(0, weighter.GetWeight(5));
    weighter.SetWeights({2, 3});
    CHECK_EQ(0, weighter.GetWeight(-1));
    CHECK_EQ(2, weighter.GetWeight(0));
    CHECK_EQ(3, weighter.GetWeight(1));
    CHECK_EQ(3, weighter.GetWeight(7));  // the last weight is the default
    CHECK_EQ(3, weighter.GetDefaultWeight());

    // Prun(weight, lo, hi) uses the remaining weights; covers only their own weight
    DualizationNode root;
    weighter.Call(root);  // computes the remaining sums: positive is unbounded (default 3 > 0)
    CHECK_EQ(inf, weighter.GetRestPositive());
    CHECK_EQ(0, weighter.GetRestNegative());
    CHECK_FALSE(weighter.Prun(1, 5, 10));
    CHECK(weighter.Prun(11, 5, 10));  // the remaining weights are not negative
    DualizationNode cover;
    cover.type = NodeType::CoverNode;
    cover.weight = 4;
    CHECK(weighter.Prun(cover, 5, 10));
    CHECK(weighter.Prun(cover, 0, 1));  // already pruned
    cover.pruned = false;
    CHECK_FALSE(weighter.Prun(cover, 4, 4));

    // Negative default weight: the negative remainder is unbounded
    weighter.Reset();
    weighter.SetWeights({1, -1});
    weighter.Call(root);
    CHECK_EQ(-inf, weighter.GetRestNegative());
    CHECK_EQ(1, weighter.GetRestPositive());
    CHECK(weighter.Prun(-5, 0, 10));  // at most -5 + 1
    CHECK_FALSE(weighter.Prun(100, 0, 10));
}

// Regression (#47): SetWeights did not invalidate the sums of the remaining weights, so a
// weighter reused with other weights pruned by the bounds of the old ones
TEST_CASE("Weighters: new weights take effect on a reused weighter") {
    auto rows = loadRows("data/15_20_275.hg");
    auto all = allCovers(rows);
    Runc::RuncGlobalState runc(Runc::RUNCM);
    runc.setMatrix(makeMatrix(rows));

    // All weights 0: the remaining positive weight is 0, every cover weighs 0
    ColumnsWeighterCallback weighter;
    weighter.SetWeights({0});
    checkKept(all, runWeighted(runc, weighter), [](Cover const&) { return (Weight) 0; }, -inf, inf);

    // Every column weighs 1 now, so a node of k columns can still reach any weight above k: with
    // the minimum 3 exactly the covers of at least 3 columns are kept
    weighter.SetWeights({1});
    weighter.SetMinWeight(3);
    checkKept(all, runWeighted(runc, weighter), [](Cover const& c) { return (Weight) c.size(); }, 3, inf);
}

TEST_CASE("MostCoverCollector with a weighter keeps the lightest or heaviest covers") {
    auto rows = loadRows("data/10_25_480.hg");
    auto all = allCovers(rows);
    Weights weights;
    for (int j = 0; j < 25; ++j)
        weights.push_back((Weight) (1 + (j * 7) % 5));
    auto weightOf = [&weights](Cover const& c) {
        Weight w = 0;
        for (int j : c)
            w += weights[j];
        return w;
    };
    std::vector<Weight> allWeights;
    for (auto const& c : all)
        allWeights.push_back(weightOf(c));
    std::sort(allWeights.begin(), allWeights.end());

    for (auto const& alg : treeAlgorithms()) {
        CAPTURE(alg.first);
        for (int order : {-1, 1}) {
            CAPTURE(order);
            ColumnsWeighterCallback weighter;
            weighter.SetWeights(weights);
            MostCoverCollector collector(&weighter);
            collector.Options = MostCoverCollectorOptions(10, 25, order);
            auto a = alg.second();
            a->setMatrix(makeMatrix(rows));
            a->InnerCallback = &weighter;
            a->CoverCallback = &collector;
            a->dualize();
            collector.Trim();
            REQUIRE_EQ(10u, collector.GetCovers().size());

            std::vector<Weight> found;
            for (auto const& c : collector.GetCovers()) {
                CHECK(all.count(sorted(c.columns)) == 1);
                CHECK_EQ(c.weight, weightOf(sorted(c.columns)));
                found.push_back(c.weight);
            }
            std::sort(found.begin(), found.end());
            std::vector<Weight> expected(order < 0 ? allWeights.begin() : allWeights.end() - 10,
                                         order < 0 ? allWeights.begin() + 10 : allWeights.end());
            CHECK(found == expected);
        }
    }

    // Without a trim order the collector keeps any MostCount covers
    MostCoverCollector collector;
    collector.Options = MostCoverCollectorOptions(5, 0, 0, false);
    CHECK_EQ(10, collector.Options.TrimSize);
    Runc::RuncGlobalState runc;
    runc.setMatrix(makeMatrix(rows));
    runc.CoverCallback = &collector;
    runc.dualize();
    CHECK(collector.GetCovers().size() <= 10u);
    collector.Trim();
    CHECK_EQ(5u, collector.GetCovers().size());
    collector.Trim(7);  // no-op: fewer covers than that
    CHECK_EQ(5u, collector.GetCovers().size());
    DualizationNode inner;
    inner.AddColumn(0);
    collector.Call(inner);  // not a cover
    CHECK_EQ(5u, collector.GetCovers().size());
    for (auto const& c : collector.GetCovers())
        CHECK(all.count(sorted(c.columns)) == 1);
}

TEST_CASE("BitCoverCollectorCallback collects covers as bit rows with their weights") {
    auto rows = loadRows("data/15_20_275.hg");
    auto all = allCovers(rows);
    for (auto const& alg : treeAlgorithms()) {
        CAPTURE(alg.first);
        ColumnsWeighterCallback weighter;
        weighter.SetWeights({1});
        BitCoverCollectorCallback collector;
        auto a = alg.second();
        a->setMatrix(makeMatrix(rows));
        a->InnerCallback = &weighter;
        a->CoverCallback = &collector;
        a->dualize();

        auto covers = toRows(collector.GetCovers());
        auto const& weights = collector.GetWeights();
        REQUIRE_EQ(all.size(), covers.size());
        REQUIRE_EQ(all.size(), weights.size());
        for (size_t i = 0; i < covers.size(); ++i) {
            CHECK(all.count(covers[i]) == 1);
            CHECK_EQ(weights[i], (Weight) covers[i].size());
        }
        CHECK(Covers(covers.begin(), covers.end()) == all);

        auto moved = collector.MoveCovers();
        auto movedWeights = collector.MoveWeights();
        CHECK_EQ((int) all.size(), moved.height());
        CHECK_EQ(all.size(), movedWeights.size());
        collector.Reset();
        CHECK_EQ(0, collector.GetCovers().height());
        CHECK(collector.GetWeights().empty());
    }
}

TEST_CASE("CoverCollector: MaxCount, Reserve, MoveCovers and Reset") {
    auto rows = loadRows("data/15_20_275.hg");
    Runc::RuncGlobalState runc;
    runc.setMatrix(makeMatrix(rows));
    CoverCollector collector;
    collector.Reserve(300);
    runc.CoverCallback = &collector;
    runc.dualize();
    CHECK_EQ(275u, collector.GetCovers().size());
    auto moved = collector.MoveCovers();
    CHECK_EQ(275u, moved.size());
    collector.Reset();
    CHECK(collector.GetCovers().empty());

    collector.MaxCount = 3;
    runc.dualize();
    CHECK_EQ(3u, collector.GetCovers().size());
    // A pruned node is not a cover
    DualizationNode node;
    node.type = NodeType::CoverNode;
    node.pruned = true;
    collector.Reset();
    collector.Call(node);
    CHECK(collector.GetCovers().empty());
}

TEST_CASE("ChainCallback and dualization_callback forward the nodes") {
    auto rows = loadRows("data/13_7_10.hg");
    Runc::RuncGlobalState runc;
    runc.setMatrix(makeMatrix(rows));

    ChainCallback<Counters, CoverCollector> chain;
    runc.CoverCallback = &chain;
    runc.dualize();
    CHECK_EQ(10, chain.callback1.cover);
    CHECK_EQ(10u, chain.callback2.GetCovers().size());

    // The second callback is skipped once the first stops the enumeration
    ChainCallback<CoverCollector, Counters> stopping;
    stopping.callback1.MaxCount = 4;
    runc.CoverCallback = &stopping;
    runc.dualize();
    CHECK_EQ(4u, stopping.callback1.GetCovers().size());
    CHECK_EQ(3, stopping.callback2.cover);

    int covers = 0;
    auto counting = dualization_callback([&covers](DualizationNode& node) { covers += node.IsCover(); });
    DualizationCallbacks callbacks = {counting};
    runc.CoverCallback = callbacks.front().get();
    runc.dualize();
    CHECK_EQ(10, covers);
}

TEST_CASE("DualizationCounters count results, steps and the maximal delay") {
    DualizationCounters counters;
    CHECK_EQ(0, counters.results);
    counters.IncInners();
    counters.IncExtra();
    counters.IncResult();  // delay 2
    CHECK_EQ(2, counters.maxDelay);
    counters.IncInners();
    counters.IncResult();  // delay 1
    CHECK_EQ(2, counters.maxDelay);
    for (int k = 0; k < 4; ++k)
        counters.IncInners();
    counters.IncResult();  // delay 4
    CHECK_EQ(3, counters.results);
    CHECK_EQ(6, counters.inners);
    CHECK_EQ(1, counters.extras);
    CHECK_EQ(4, counters.maxDelay);
    CHECK_EQ(0, counters.currentDelay);
    auto text = counters.ToString();
    CHECK(text.find("inner:6") != std::string::npos);
    CHECK(text.find("count 3") != std::string::npos);
    CHECK(text.find("max delay: 4") != std::string::npos);
    counters.Reset();
    CHECK_EQ(0, counters.results);
    CHECK_EQ(0, counters.inners);
    CHECK_EQ(0, counters.maxDelay);
}

TEST_CASE("File writers write one line per cover") {
    std::filesystem::create_directories("temp");
    auto rows = loadRows("data/15_20_275.hg");
    // Covers in the order of the enumeration, with their weights
    ColumnsWeighterCallback weighter;
    Weights weights;
    for (int j = 0; j < 20; ++j)
        weights.push_back(0.1f * (Weight) j + 0.2f);
    weighter.SetWeights(weights);
    CoverCollector reference;
    Runc::RuncGlobalState runc;
    runc.setMatrix(makeMatrix(rows));
    runc.InnerCallback = &weighter;
    runc.CoverCallback = &reference;
    runc.dualize();
    auto const& covers = reference.GetCovers();
    REQUIRE_EQ(275u, covers.size());

    struct Case {
        std::string name;
        std::function<Writer()> create;
        std::function<std::string(WeightedCover const&)> expected;
    };
    std::vector<Case> cases = {
            {"all", makeWriter<AllColumnsFileWriterCallback>, [](WeightedCover const& c) { return join(c.columns); }},
            {"ordered",
             makeWriter<OrderedColumnsFileWriterCallback>,
             [](WeightedCover const& c) { return join(sorted(c.columns)); }},
            {"first",
             makeWriter<FirstColumnFileWriterCallback>,
             [](WeightedCover const& c) { return std::to_string(c.columns.front()); }},
            {"last",
             makeWriter<LastColumnFileWriterCallback>,
             [](WeightedCover const& c) { return std::to_string(c.columns.back()); }},
            {"size",
             makeWriter<SizeFileWriterCallback>,
             [](WeightedCover const& c) { return std::to_string(c.columns.size()); }},
            {"min",
             makeWriter<MinColumnFileWriterCallback>,
             [](WeightedCover const& c) { return std::to_string(sorted(c.columns).front()); }},
            {"max",
             makeWriter<MaxColumnFileWriterCallback>,
             [](WeightedCover const& c) { return std::to_string(sorted(c.columns).back()); }},
            {"weight",
             makeWriter<WeightFileWriterCallback>,
             [](WeightedCover const& c) {
                 // 5 digits after the point, rounded to the nearest; none for a whole number
                 double w = c.weight;
                 if (std::floor(w) == w)
                     return std::to_string((long long) w);
                 char buffer[64];
                 std::snprintf(buffer, sizeof buffer, "%.5f", w);
                 return std::string(buffer);
             }},
    };

    for (auto const& c : cases) {
        CAPTURE(c.name);
        auto file = "temp/cov_writer_" + c.name + ".txt";
        {
            auto writer = c.create();
            writer.file->CreateOutputFile(file);
            runc.CoverCallback = writer.callback.get();
            runc.dualize();
            // Inner nodes are not written
            DualizationNode inner;
            inner.AddColumn(1);
            runc.CoverCallback->Call(inner);
            writer.file->CloseOutputFile();
        }
        auto lines = readLines(file);
        REQUIRE_EQ(covers.size(), lines.size());
        for (size_t k = 0; k < covers.size(); ++k)
            CHECK_EQ(c.expected(covers[k]), lines[k]);
    }

    // The empty cover: an empty line for the writers of one column
    for (auto const& c : cases) {
        CAPTURE(c.name);
        auto file = "temp/cov_writer_empty_" + c.name + ".txt";
        {
            auto writer = c.create();
            writer.file->CreateOutputFile(file);
            Runc::RuncGlobalState empty;
            empty.setMatrix(bit_matrix());
            empty.CoverCallback = writer.callback.get();
            empty.dualize();
        }
        auto lines = readLines(file);
        REQUIRE_EQ(1u, lines.size());
        CHECK_EQ(lines[0], c.name == "size" || c.name == "weight" ? "0" : "");
    }
}

TEST_CASE("Restrictions: setRestrictions and loadRestrictions build the same matrix") {
    struct Exposed : Runc::RuncGlobalState {
        bit_matrix const& restrictions() const {
            return _restrictions;
        }
    };
    Exposed set, loaded;
    auto rows = loadRows("data/6_6_4.hg");
    set.setRestrictions(makeMatrix(rows));
    loaded.loadRestrictions("data/6_6_4.hg");
    REQUIRE_EQ(set.restrictions().height(), loaded.restrictions().height());
    CHECK(toRows(set.restrictions()) == toRows(loaded.restrictions()));
    // Row j: the union of the rows with column j
    int n = widthOf(rows);
    CHECK_EQ(n, set.restrictions().height());
    for (int j = 0; j < n; ++j) {
        Cover expected;
        for (int k = 0; k < n; ++k)
            for (auto const& r : rows)
                if (std::count(r.begin(), r.end(), j) && std::count(r.begin(), r.end(), k)) {
                    expected.push_back(k);
                    break;
                }
        CHECK(toRows(set.restrictions())[j] == expected);
    }
}

TEST_CASE("CoveredRowsWeighter: TargetToMin and TargetToMax with epsilon") {
    auto rows = loadRows("data/15_20_275.hg");
    auto all = allCovers(rows);
    // Every cover hits every row, so the target rows are the input rows restricted to columns 0..4:
    // a cover weighs the rows it hits with these columns, row i weighs i % 3 + 1
    bit_matrix target;
    target.zero((int) rows.size(), widthOf(rows));
    Weights rowWeights;
    for (size_t i = 0; i < rows.size(); ++i) {
        rowWeights.push_back((Weight) (i % 3 + 1));
        for (int j : rows[i])
            if (j < 5)
                target.get_row((int) i).set(j);
    }
    auto weightOf = [&](Cover const& c) {
        Weight w = 0;
        for (size_t i = 0; i < rows.size(); ++i)
            for (int j : c)
                if (j < 5 && std::count(rows[i].begin(), rows[i].end(), j)) {
                    w += rowWeights[i];
                    break;
                }
        return w;
    };
    Weight lightest = inf, heaviest = -inf;
    for (auto const& c : all) {
        lightest = std::min(lightest, weightOf(c));
        heaviest = std::max(heaviest, weightOf(c));
    }
    REQUIRE(lightest < heaviest);

    for (bool toMin : {true, false}) {
        for (Weight eps : {0.0f, 2.0f}) {
            CAPTURE(toMin);
            CAPTURE(eps);
            CoveredRowsWeighter weighter;
            CoveredRowsWeighter::Options options;
            options.TargetRows = target.get_rows();
            options.RowsWeights = rowWeights;
            weighter.SetOptions(options);
            weighter.SetEpsilon(eps);
            if (toMin)
                weighter.TargetToMin();
            else
                weighter.TargetToMax();

            struct Chain : IDualizationCallback {
                CoveredRowsWeighter* weighter = nullptr;
                CoverCollector collector;
                void Call(DualizationNode& node) override {
                    weighter->Call(node);
                    collector.Call(node);
                }
            } chain;
            chain.weighter = &weighter;
            Runc::RuncGlobalState runc;
            runc.setMatrix(makeMatrix(rows));
            runc.InnerCallback = &weighter;
            runc.CoverCallback = &chain;
            runc.dualize();

            WeightedCoverMap found;
            for (auto const& c : chain.collector.GetCovers())
                found.emplace(sorted(c.columns), c.weight);
            CHECK(found.size() < all.size());
            for (auto const& c : found)
                CHECK_EQ(c.second, weightOf(c.first));
            // Every cover within eps of the extreme weight is found
            for (auto const& c : all) {
                Weight w = weightOf(c);
                if (toMin ? w <= lightest + eps : w >= heaviest - eps)
                    CHECK_MESSAGE(found.count(c), "a cover near the extreme weight was pruned");
            }
        }
    }
}

TEST_CASE("BoolMatrixPreparation removes the rows that contain another row") {
    std::mt19937 random(4703);
    for (int it = 0; it < 200; ++it) {
        int n = 1 + random() % 150;  // several 64-bit blocks
        auto rows = randomRows(random, 1 + random() % 12, n, 0);
        if (it % 3 == 0)
            rows.push_back(rows.front());  // equal rows: one of them stays
        CAPTURE(it);
        bit_matrix matrix = makeMatrix(rows, n);

        auto chunks = matrix.get_rows();
        BoolMatrixPreparation::RemoveDependentRows(chunks);
        Covers expected;
        for (auto const& r : rows) {
            auto sr = sorted(r);
            bool dependent = false;
            for (auto const& other : rows) {
                auto so = sorted(other);
                if (so != sr && std::includes(sr.begin(), sr.end(), so.begin(), so.end()))
                    dependent = true;
            }
            if (!dependent)
                expected.insert(sr);
        }
        std::multiset<Cover> kept;
        for (auto const& chunk : chunks)
            kept.insert(maskedColumns(chunk, 1));
        CHECK(Covers(kept.begin(), kept.end()) == expected);
        CHECK_EQ(kept.size(), expected.size());  // of equal rows one is kept

        // With a mask the rows are compared on the masked (even) columns only
        bit_vector mask(n);
        for (int j = 0; j < n; j += 2)
            mask.set(j);
        auto masked = matrix.get_rows();
        BoolMatrixPreparation::RemoveDependentRows(masked, bit_mask(mask));
        std::multiset<Cover> maskedKept;
        for (auto const& chunk : masked)
            maskedKept.insert(maskedColumns(chunk, 2));
        Covers maskedExpected;
        for (auto const& r : rows) {
            Cover even;
            for (int j : sorted(r))
                if (j % 2 == 0)
                    even.push_back(j);
            maskedExpected.insert(even);
        }
        // The minimal ones of the masked rows remain, once each
        Covers minimal;
        for (auto const& c : maskedExpected) {
            bool dependent = false;
            for (auto const& d : maskedExpected)
                if (d != c && std::includes(c.begin(), c.end(), d.begin(), d.end()))
                    dependent = true;
            if (!dependent)
                minimal.insert(c);
        }
        CHECK(Covers(maskedKept.begin(), maskedKept.end()) == minimal);
        CHECK_EQ(maskedKept.size(), minimal.size());
    }
}

// Regression (#47): WriteNumber truncated the fraction, so the float weight 0.7f (0.69999998...)
// was written as 0.69999
TEST_CASE("WeightFileWriterCallback rounds the weight to 5 digits") {
    std::filesystem::create_directories("temp");
    char const* file = "temp/cov_writer_rounding.txt";
    {
        WeightFileWriterCallback writer;
        writer.CreateOutputFile(file);
        for (Weight w : {0.7f, 2.5f, -1.25f, 3.0f, 0.000004f, 1.999999f, -0.3f, 12345.67f}) {
            DualizationNode node;
            node.type = NodeType::CoverNode;
            node.weight = w;
            writer.Call(node);
        }
    }
    std::vector<std::string> expected =
            {"0.70000", "2.50000", "-1.25000", "3", "0.00000", "2.00000", "-0.30000", "12345.66992"};
    CHECK(readLines(file) == expected);
}
