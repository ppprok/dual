// Regression tests for memory and logic defects in common containers (issue #21, audit #11).

#include <doctest.h>

#include <vector>

#if defined(__GLIBC__)
#    include <malloc.h>
#endif

#include <dual/bit_mask.h>
#include <dual/bit_matrix.h>
#include <dual/bit_vector.h>
#include <dual/blob.h>
#include <dual/chunk.h>

using namespace dual;

namespace {

bit_vector make_bits(std::vector<int> const& bits, int n = 64) {
    bit_vector v(n);
    for (int b : bits)
        v.set(b);
    return v;
}

}  // namespace

// Audit finding 1: chunk::operator+=/-= truncated the pointer offset to int.
// Uses real heap buffers: on 64-bit targets their addresses divided by sizeof(T) exceed INT_MAX, so the old
// truncation is caught. The rebase in chunk itself still relies on flat pointer arithmetic (base - (T*)0),
// which is formally UB; a cleaner rebase is a follow-up for #12.
TEST_CASE("chunk_rebase_keeps_full_pointer_offset") {
    using T = unsigned int;
    std::vector<T> oldBuf(8), newBuf(8);
    T* oldBase = oldBuf.data();
    T* newBase = newBuf.data();

    chunk<T> c(oldBase + 3, oldBase + 5);
    c -= oldBase;
    c += newBase;

    CHECK(c.begin == newBase + 3);
    CHECK(c.end == newBase + 5);
}

// Audit finding 1: bit_matrix::reserve moved rows to a wrong address after realloc of a large buffer.
TEST_CASE("bit_matrix_larger_than_128KB_keeps_rows") {
    const int m = 20000;
    const int n = 2000;
    bit_matrix bm;
    for (int i = 0; i < m; ++i)
        bm.set(i, (i * 7) % n);

    REQUIRE_EQ(m, bm.height());
    bool ok = true;
    for (int i = 0; i < m && ok; ++i)
        ok = bm.get(i, (i * 7) % n) && bm.get_row(i).count() == 1;
    CHECK(ok);
}

// Audit finding 2: blob::resize on an empty blob left size() == 0.
TEST_CASE("blob_resize_from_empty") {
    blob<int> b;
    b.resize(5);
    CHECK_EQ(5, b.size());
    CHECK(b.end() == b.begin() + 5);
    for (int i = 0; i != 5; ++i)
        CHECK_EQ(0, b[i]);
}

// Audit finding 2: bit_matrix capacity collapsed after the first reserve and add_bit shrank the buffer.
TEST_CASE("bit_matrix_zero_then_set_last_row") {
    bit_matrix bm;
    bm.zero(7, 7);
    bm.set(6, 1);
    bm.set(0, 6);
    CHECK(bm.get(6, 1));
    CHECK(bm.get(0, 6));
    CHECK_EQ(7, bm.height());
}

// Audit finding 13: bit_mask::subset_cmp reported equal or empty masked blocks as incomparable.
TEST_CASE("bit_mask_subset_cmp_equal_and_empty_blocks") {
    bit_vector maskBits = make_bits({}, 64);
    for (int i = 0; i != 64; ++i)
        maskBits.set(i);
    bit_mask mask(maskBits);

    auto a = make_bits({0, 40});
    auto b = make_bits({0});
    CHECK_EQ(1, mask.subset_cmp(a, b));
    CHECK_EQ(-1, mask.subset_cmp(b, a));

    auto c = make_bits({40, 41});
    auto d = make_bits({41});
    CHECK_EQ(1, mask.subset_cmp(c, d));
    CHECK_EQ(-1, mask.subset_cmp(d, c));

    // Equal rows must be comparable; the sign returned for equal rows is not part of the contract.
    auto e = make_bits({0, 40});
    CHECK_NE(0, mask.subset_cmp(a, e));

    auto f = make_bits({1, 40});
    CHECK_EQ(0, mask.subset_cmp(a, f));
}

// Audit finding 14: bit_chunk::get_last used low_bit_number, so find_last returned a non-last bit.
TEST_CASE("bit_chunk_find_last_returns_highest_bit") {
    CHECK_EQ(2, make_bits({1, 2}).find_last());
    CHECK_EQ(31, make_bits({28, 29, 30, 31}).find_last());
    CHECK_EQ(60, make_bits({33, 60}).find_last());
    CHECK_EQ(13, make_bits({9, 13}).find_last());
    CHECK_EQ(5, make_bits({0, 5}).find_last());

    bit_vector maskBits = make_bits({}, 64);
    for (int i = 0; i != 64; ++i)
        maskBits.set(i);
    bit_mask mask(maskBits);
    CHECK_EQ(2, mask.find_last(make_bits({1, 2})));
}

// Audit finding 17: chunk::resize(s, value) shadowed end and did not change the size.
TEST_CASE("chunk_resize_with_value") {
    int buf[4] = {0, 0, 0, 0};
    chunk<int> c(buf, buf);
    c.resize(3, 7);
    CHECK_EQ(3, c.size());
    CHECK_EQ(7, buf[0]);
    CHECK_EQ(7, buf[2]);
    CHECK_EQ(0, buf[3]);
}

// Audit finding 17: bit_vector::zero passed the number of blocks to malloc instead of bytes.
TEST_CASE("bit_vector_zero_allocates_enough_bytes") {
    bit_vector v;
    v.zero(256);
#if defined(__GLIBC__)
    CHECK(malloc_usable_size(v.begin) >= 8 * sizeof(bit_chunk::Block));
#endif
    for (int i = 0; i != 256; ++i)
        v.set(i);
    CHECK_EQ(256, v.count());
    CHECK_EQ(255, v.find_last());

    v.zero(1024);
#if defined(__GLIBC__)
    CHECK(malloc_usable_size(v.begin) >= 32 * sizeof(bit_chunk::Block));
#endif
    for (int i = 0; i != 1024; ++i)
        v.set(i);
    CHECK_EQ(1024, v.count());
}
