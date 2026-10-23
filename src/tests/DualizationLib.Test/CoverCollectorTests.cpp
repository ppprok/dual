// Pins the documented semantics of MostCoverCollectorOptions::TrimOrder (issue #36).

#include <doctest.h>

#include <algorithm>
#include <vector>

#include <dual/most_cover_collector.h>

using namespace dual;

namespace {

// Feeds covers with the given weights; the cover of weight w has w + 1 columns unless
// columnCounts gives the number of columns.
void feed(MostCoverCollector& collector,
          std::vector<Weight> const& weights,
          std::vector<int> const& columnCounts = {}) {
    for (size_t i = 0; i != weights.size(); ++i) {
        DualizationNode node;
        int k = columnCounts.empty() ? (int) weights[i] + 1 : columnCounts[i];
        for (int j = 0; j != k; ++j)
            node.AddColumn(j);
        node.type = NodeType::CoverNode;
        node.weight = weights[i];
        collector.Call(node);
    }
}

std::vector<Weight> sorted_weights(MostCoverCollector const& collector) {
    std::vector<Weight> w;
    for (auto const& c : collector.GetCovers())
        w.push_back(c.weight);
    std::sort(w.begin(), w.end());
    return w;
}

}  // namespace

TEST_CASE("most_cover_collector_negative_trim_order_keeps_lightest_covers") {
    MostCoverCollector collector;
    collector.Options = MostCoverCollectorOptions(2, 4, -1);
    feed(collector, {5, 3, 8, 1, 7, 6, 2});
    collector.Trim();
    CHECK_EQ(std::vector<Weight>{1, 2}, sorted_weights(collector));
}

TEST_CASE("most_cover_collector_positive_trim_order_keeps_heaviest_covers") {
    MostCoverCollector collector;
    collector.Options = MostCoverCollectorOptions(2, 4, 1);
    feed(collector, {5, 3, 8, 1, 7, 6, 2});
    collector.Trim();
    CHECK_EQ(std::vector<Weight>{7, 8}, sorted_weights(collector));
}

TEST_CASE("most_cover_collector_trim_order_breaks_ties_by_fewer_columns") {
    for (int order : {-1, 1}) {
        CAPTURE(order);
        MostCoverCollector collector;
        collector.Options = MostCoverCollectorOptions(1, 2, order);
        feed(collector, {4, 4, 4}, {3, 1, 2});
        collector.Trim();
        REQUIRE_EQ(1u, collector.GetCovers().size());
        CHECK_EQ(1u, collector.GetCovers().front().columns.size());
    }
}
