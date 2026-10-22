// Regression tests for latent defects in common helpers found while documenting (issue #36).

#include <doctest.h>

#include "bit_matrix.h"
#include "bit_vector.h"
#include "graph_algorithms.h"

TEST_CASE("remove_rows_returns_zero_when_no_row_matches") {
    bit_matrix m;
    m.zero(3, 4);
    m.set(0, 0);
    m.set(1, 1);
    m.set(2, 0);
    m.set(2, 3);

    CHECK_EQ(0, m.remove_zero_rows());
    CHECK_EQ(3, m.height());

    CHECK_EQ(0, m.remove_covered_rows(bit_vector(4)));
    CHECK_EQ(3, m.height());

    bit_vector c(4);
    c.set(0);
    CHECK_EQ(2, m.remove_covered_rows(c));
    REQUIRE_EQ(1, m.height());
    CHECK(m.get(0, 1));

    bit_matrix empty;
    CHECK_EQ(0, empty.remove_zero_rows());
}

// Rows of more than one block: the old loop ran over size() (blocks in use), past the last row.
TEST_CASE("is_transitive_closed_checks_rows_wider_than_a_block") {
    int const n = 40;
    bit_matrix adj;
    adj.zero(n, n);
    for (int i = 0; i + 1 < n; ++i)
        adj.set(i, i + 1);

    CHECK_FALSE(is_transitive_closed(adj));

    transitive_closure(adj);
    CHECK(is_transitive_closed(adj));
    CHECK_EQ(n * (n - 1) / 2, edges_number(adj));

    adj.reset(n - 3, n - 1);
    CHECK_FALSE(is_transitive_closed(adj));
}
