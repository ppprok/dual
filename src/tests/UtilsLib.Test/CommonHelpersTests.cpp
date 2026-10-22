// Regression tests for latent defects in common helpers found while documenting (issue #36).

#include <doctest.h>

#include <vector>

#include "bit_mask.h"
#include "bit_matrix.h"
#include "bit_vector.h"
#include "containers.h"
#include "graph_algorithms.h"
#include "string_utils.h"

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

TEST_CASE("streq_compares_two_ranges_by_length_and_content") {
    char const a[] = "abcabd";
    CHECK(streq(a, a + 2, a + 3, a + 5));
    CHECK_FALSE(streq(a, a + 3, a + 3, a + 6));
    CHECK_FALSE(streq(a, a + 2, a + 3, a + 6));
    CHECK_FALSE(streq(a, a + 3, a + 3, a + 5));
    CHECK(streq(a, a, a + 3, a + 3));
}

// exclude called an undefined remove(), so instantiating it failed to compile.
TEST_CASE("exclude_removes_listed_elements_keeping_order") {
    std::vector<int> c{5, 1, 4, 2, 3, 4};
    std::vector<int> const excl{4, 1, 7};
    exclude(c, excl);
    CHECK_EQ(std::vector<int>{5, 2, 3}, c);

    exclude(c, std::vector<int>{});
    CHECK_EQ(std::vector<int>{5, 2, 3}, c);
}

// The loop stopped at e1 != b2.end instead of e2 != b2.end, so a shorter b2 was read past its end.
TEST_CASE("bit_mask_intersects_stops_at_the_end_of_the_second_chunk") {
    bit_chunk::Block maskBlocks[3] = {0, ~0u, ~0u};
    bit_chunk::Block b1Blocks[3] = {0, ~0u, ~0u};
    // Only the first block belongs to b2; the following ones must not be read.
    bit_chunk::Block b2Blocks[3] = {0, ~0u, ~0u};

    bit_mask mask(bit_chunk(maskBlocks, maskBlocks + 3));
    bit_chunk b1(b1Blocks, b1Blocks + 3);
    bit_chunk b2(b2Blocks, b2Blocks + 1);

    CHECK_FALSE(mask.intersects(b1, b2));
    CHECK_FALSE(mask.intersects(b2, b1));

    b2Blocks[0] = 1;
    maskBlocks[0] = 1;
    CHECK_FALSE(mask.intersects(b1, b2));
    b1Blocks[0] = 1;
    CHECK(mask.intersects(b1, b2));
    CHECK(mask.intersects(b2, b1));
}
