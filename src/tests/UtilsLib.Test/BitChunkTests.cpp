#include <doctest.h>

#include <algorithm>
#include <random>
#include <vector>

#include <dual/bit_mask.h>
#include <dual/bit_vector.h>
#include <dual/containers.h>

using namespace dual;

TEST_CASE("get_set_bits_test") {
    bit_vector bits(1000);

    bits.set(44);
    bits.set(64);
    bits.set(1000);
    bits.set(511);
    bits.set(999);
    bits.reset(1000);

    CHECK(bits.get(44));
    CHECK(bits.get(64));
    CHECK(bits.get(999));
    CHECK(bits.get(511));
    CHECK(!bits.get(1000));

    std::vector<int> actual;
    bits.get_bits(actual);

    std::vector<int> expected = {(44), (64), (511), (999)};

    CHECK(equal(actual, expected));

    for (int i = 0; i != 1024; ++i) {
        if (contains(actual, i))
            continue;
        CHECK_FALSE(bits.get(i));
    }
}

TEST_CASE("assing_bits_test") {
    bit_vector bits1(1000);
    bit_vector bits2(1000);

    bits1.set(44);
    bits1.set(64);
    bits1.set(1000);
    bits1.set(511);
    bits1.set(999);

    bits2.assign(bits1);

    CHECK(bits1 == bits2);
}

TEST_CASE("assing_and_bits_test") {
    bit_vector bits1(1000);
    bit_vector bits2(1000);

    bits1.set(44);
    bits1.set(64);
    bits1.set(1000);
    bits1.set(511);
    bits1.set(999);
    bits1.set(399);

    bits2.set(4);
    bits2.set(64);
    bits2.set(100);
    bits2.set(511);
    bits2.set(999);

    bit_vector bits3(1000);

    bits3.assign_and(bits1, bits2);

    std::vector<int> actual;
    bits3.get_bits(actual);

    std::vector<int> expected = {64, 511, 999};

    CHECK(equal(actual, expected));
}

// Test enumeration of the differing bits of two bit vectors
TEST_CASE("find_diff_bits_test") {
    bit_vector b1(1000);
    bit_vector b2(1000);

    b1.set(44);
    b1.set(31);
    b1.set(678);
    b1.set(549);
    b1.set(78);
    b1.trim();

    b2.set(440);
    b2.set(31);
    b2.set(78);
    b2.set(59);
    b2.trim();

    bit_mask m2(b2);

    std::vector<int> actual;

    for (int j = m2.find_first_diff(b1); j != -1; j = m2.find_next_diff(b1, j)) {
        actual.push_back(j);
    }

    std::vector<int> expected = {(44), (59), (440), (549), (678)};

    CHECK(equal(expected, actual));
}

namespace {

// Bit set of n random blocks; some blocks (also trailing ones) are zero so that trimming is exercised.
// One extra zero block keeps data() non-null for n = 0 (a chunk with null bounds is not valid()).
std::vector<bit_chunk::Block> randomBlocks(std::mt19937_64& g, int n) {
    std::vector<bit_chunk::Block> v(n + 1, 0);
    for (auto& b : v) {
        if (&b == &v.back())
            break;
        switch (g() % 4) {
            case 0:
                b = 0;
                break;
            case 1:
                b = bit_chunk::Block(1) << (g() % bit_chunk::bits_per_block);
                break;
            default:
                b = g();
        }
    }
    return v;
}

// Reference semantics: the set bits of a chunk
std::vector<int> bitsOf(bit_chunk c) {
    std::vector<int> r;
    for (int i = 0; i < c.bit_size(); ++i)
        if (c.get(i))
            r.push_back(i);
    return r;
}

std::vector<int> combine(std::vector<int> const& a, std::vector<int> const& b, int op) {
    std::vector<int> r;
    for (int i = 0; i < 64 * 64; ++i) {
        bool x = std::binary_search(a.begin(), a.end(), i);
        bool y = std::binary_search(b.begin(), b.end(), i);
        if (op == 0 ? (x && y) : op == 1 ? (x || y) : (x && !y))
            r.push_back(i);
    }
    return r;
}

bool trimmed(bit_chunk c) {
    return c.empty() || c.end[-1] != 0;
}

}  // namespace

// The block loops switch from a scalar loop to a vectorizable loop plus trim at
// bit_chunk::vector_min_blocks blocks; check both against a reference, also when the destination
// aliases an argument.
TEST_CASE("bit_chunk_block_loops_match_reference") {
    std::mt19937_64 g(59);
    for (int t = 0; t < 400; ++t) {
        int n1 = int(g() % 36);
        int n2 = int(g() % 36);
        auto v1 = randomBlocks(g, n1);
        auto v2 = randomBlocks(g, n2);
        bit_chunk r1(v1.data(), v1.data() + n1);
        bit_chunk r2(v2.data(), v2.data() + n2);
        auto b1 = bitsOf(r1);
        auto b2 = bitsOf(r2);

        std::vector<bit_chunk::Block> out(64, ~bit_chunk::Block(0));
        bit_chunk d(out.data(), out.data());

        d.assign_and(r1, r2);
        CHECK(bitsOf(d) == combine(b1, b2, 0));
        CHECK(trimmed(d));
        d.assign_or(r1, r2);
        CHECK(bitsOf(d) == combine(b1, b2, 1));
        CHECK(trimmed(d));
        d.assign_sub(r1, r2);
        CHECK(bitsOf(d) == combine(b1, b2, 2));
        CHECK(trimmed(d));
        d.assign(r1);
        CHECK(bitsOf(d) == b1);
        CHECK(trimmed(d));

        CHECK(r1.count() == (int) b1.size());
        if (b1.size() <= 3)
            CHECK(r1.count(3) == (int) b1.size());
        else
            CHECK(r1.count(3) > 3);
        CHECK(r1.intersects(r2) == !combine(b1, b2, 0).empty());
        CHECK(r1.is_subset_of(r2) == combine(b1, b2, 2).empty());
        CHECK((r1 == r2) == (b1 == b2));
        CHECK((r1 == r2) == (!(r1 < r2) && !(r2 < r1)));

        bit_mask mask(r2);
        CHECK(mask.count(r1) == (int) combine(b1, b2, 0).size());
        CHECK(mask.none(r1) == combine(b1, b2, 0).empty());
        CHECK(mask.find_first(r1) == (combine(b1, b2, 0).empty() ? -1 : combine(b1, b2, 0).front()));

        // Aliasing: the destination is one of the arguments
        auto a1 = v1;
        bit_chunk x(a1.data(), a1.data() + n1);
        x.assign_and(x, r2);
        CHECK(bitsOf(x) == combine(b1, b2, 0));
        a1 = v1;
        a1.resize(64, 0);
        x = bit_chunk(a1.data(), a1.data() + n1);
        x.assign_or(r2, x);
        CHECK(bitsOf(x) == combine(b1, b2, 1));
        a1 = v1;
        x = bit_chunk(a1.data(), a1.data() + n1);
        x.assign_sub(x, r2);
        CHECK(bitsOf(x) == combine(b1, b2, 2));
        a1 = v1;
        x = bit_chunk(a1.data(), a1.data() + n1);
        x.flip();
        CHECK(x.count() == n1 * bit_chunk::bits_per_block - (int) b1.size());
        CHECK(trimmed(x));
    }
}
