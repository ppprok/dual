// Regression tests for latent defects in common helpers found while documenting (issue #36).

#include <doctest.h>

#include <cstring>
#include <new>
#include <utility>
#include <vector>

#include <dual/bit_mask.h>
#include <dual/bit_matrix.h>
#include <dual/bit_vector.h>
#include <dual/blob.h>
#include <dual/containers.h>
#include <dual/raw_vector.h>

#include "common/graph_algorithms.h"
#include "common/string_utils.h"

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

namespace {

std::vector<int> to_std(raw_vector<int> const& v) {
    return std::vector<int>(v.begin(), v.end());
}

}  // namespace

// erase computed the count as b - e (negative): an assert fired in Debug, the end moved forward otherwise.
TEST_CASE("raw_vector_erase_removes_elements") {
    raw_vector<int> v;
    for (int i = 0; i != 6; ++i)
        v.push_back(i);

    v.erase(1);
    CHECK_EQ(std::vector<int>{0, 2, 3, 4, 5}, to_std(v));

    v.erase(v.begin() + 1, v.begin() + 3);
    CHECK_EQ(std::vector<int>{0, 4, 5}, to_std(v));

    v.erase(v.end() - 1);
    CHECK_EQ(std::vector<int>{0, 4}, to_std(v));

    v.erase(v.begin(), v.begin());
    CHECK_EQ(std::vector<int>{0, 4}, to_std(v));
}

namespace {

// Constructs T in storage filled with a garbage pattern, so a constructor that leaves members
// uninitialised is caught deterministically (they keep the pattern instead of happening to be 0).
template<typename T>
struct dirty_storage {
    alignas(T) unsigned char bytes[sizeof(T)];

    dirty_storage() {
        std::memset(bytes, 0xAB, sizeof(bytes));
    }

    template<typename... Args>
    T& construct(Args&&... args) {
        return *new (bytes) T(std::forward<Args>(args)...);
    }
};

}  // namespace

// The copy constructor initialised nothing for an empty source and grew a non-empty copy from
// uninitialised members.
TEST_CASE("raw_vector_copy_constructor_initialises_members") {
    raw_vector<int> empty;
    dirty_storage<raw_vector<int>> s1;
    auto& c1 = s1.construct(empty);
    CHECK(c1.empty());
    CHECK_EQ(0, c1.capacity());
    CHECK_EQ(nullptr, c1.begin());
    c1.~raw_vector();

    raw_vector<int> full;
    for (int i = 0; i != 3; ++i)
        full.push_back(i * 10);
    dirty_storage<raw_vector<int>> s2;
    auto& c2 = s2.construct(full);
    CHECK_EQ(std::vector<int>{0, 10, 20}, to_std(c2));
    c2.push_back(30);
    CHECK_EQ(std::vector<int>{0, 10, 20, 30}, to_std(c2));
    c2.~raw_vector();
}

TEST_CASE("raw_vector_range_constructors_initialise_members") {
    std::vector<int> const src{1, 2, 3};

    dirty_storage<raw_vector<int>> s1;
    auto& v1 = s1.construct(src.begin(), src.end());
    CHECK_EQ(src, to_std(v1));
    v1.~raw_vector();

    dirty_storage<raw_vector<int>> s2;
    auto& v2 = s2.construct(src.data(), src.data() + src.size());
    CHECK_EQ(src, to_std(v2));
    v2.~raw_vector();

    dirty_storage<raw_vector<int>> s3;
    auto& v3 = s3.construct(src.data(), src.data());
    CHECK(v3.empty());
    v3.~raw_vector();
}

// blob(Iter, Iter) and blob(T const*, T const*) resized from uninitialised _begin/_end.
TEST_CASE("blob_range_constructors_initialise_members") {
    std::vector<int> const src{4, 5, 6};

    dirty_storage<blob<int>> s1;
    auto& b1 = s1.construct(src.begin(), src.end());
    CHECK_EQ(src, std::vector<int>(b1.begin(), b1.end()));
    b1.~blob();

    dirty_storage<blob<int>> s2;
    auto& b2 = s2.construct(src.data(), src.data() + src.size());
    CHECK_EQ(src, std::vector<int>(b2.begin(), b2.end()));
    b2.~blob();

    dirty_storage<blob<int>> s3;
    auto& b3 = s3.construct(src.data(), src.data());
    CHECK(b3.empty());
    CHECK_EQ(nullptr, b3.begin());
    b3.~blob();
}
