#include <doctest.h>

#include "bit_matrix.h"
#include "graph_algorithms.h"

TEST_CASE("TopologicalOrderTest") {
    bit_matrix adj;
    adj.zero(7, 7);
    adj.set(0, 2);
    adj.set(2, 3);
    adj.set(1, 2);
    adj.set(1, 3);
    adj.set(2, 4);
    adj.set(5, 4);
    adj.set(4, 6);

    std::vector<int> order;
    topological_order(adj, order);
    CHECK_EQ(7, (int) order.size());
    CHECK(is_topological_order(adj, order));

    std::iter_swap(order.begin(), order.rbegin());
    CHECK_FALSE(is_topological_order(adj, order));
}

TEST_CASE("TopologicalOrderNotAcyclicTest") {
    bit_matrix adj;
    adj.zero(7, 7);
    adj.set(0, 2);
    adj.set(2, 3);
    adj.set(1, 2);
    adj.set(1, 3);
    adj.set(2, 4);
    adj.set(5, 4);
    adj.set(4, 6);
    adj.set(6, 1);

    std::vector<int> order;

    try {
        topological_order(adj, order);
        FAIL("");
    }
    catch (std::exception&) {
    }
}

TEST_CASE("TransitiveClosureTest") {
    bit_matrix adj;
    adj.zero(7, 7);
    adj.set(0, 2);
    adj.set(2, 3);
    adj.set(1, 2);
    adj.set(1, 3);
    adj.set(2, 4);
    adj.set(5, 4);
    adj.set(4, 6);

    CHECK_FALSE(is_transitive_closed(adj));

    transitive_closure(adj);

    CHECK(is_transitive_closed(adj));

    CHECK_EQ(7 + 7, edges_number(adj));
}
