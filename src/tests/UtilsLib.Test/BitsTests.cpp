#include <doctest.h>

#include <vector>

#include <dual/bits.h>
#include <dual/containers.h>

using namespace dual;

TEST_CASE("bit_count_test") {
    bits32 bits = 0x12345678;
    CHECK_EQ(13, bits_count(bits));

    bits32 bits1 = 0x87654321;
    CHECK_EQ(13, bits_count(bits1));

    bits64 bits2 = 0x8765432112345678;
    CHECK_EQ(26, bits_count(bits2));
}

TEST_CASE("one_bit_count_test") {
    for (int i = 0; i != 32; ++i) {
        bits32 bits = 1 << i;
        CHECK_EQ(1, bits_count(bits));
    }
}

TEST_CASE("low_bit_iterator_test") {
    std::vector<int> i;

    bits32 bits = 0x12345678;

    for (auto lb = low_bit<bits32>(bits); lb; ++lb)
        i.push_back(lb);

    CHECK_EQ(13, (int) i.size());
    std::vector<int> i0 = {(3), (4), (5), (6), (9), (10), (12), (14), (18), (20), (21), (25), (28)};
    CHECK(equal(i0, i));
}

TEST_CASE("bit_index_test") {
    for (int i = 0; i != 64; ++i) {
        bits64 one = bits64(1) << i;
        CHECK_EQ(i, lowest_bit_index(one));
        CHECK_EQ(i, highest_bit_index(one));
        CHECK_EQ(0, lowest_bit_index(one | 1u));
        CHECK_EQ(63, highest_bit_index(one | (bits64(1) << 63)));
        if (i < 32) {
            bits32 one32 = bits32(1) << i;
            CHECK_EQ(i, lowest_bit_index(one32));
            CHECK_EQ(i, highest_bit_index(one32));
            CHECK_EQ(31, highest_bit_index(bits32(one32 | 0x80000000u)));
        }
    }
    CHECK_EQ(4, lowest_bit_index(bits64(0xf0f0000000000010ULL)));
    CHECK_EQ(63, highest_bit_index(bits64(0xf0f0000000000010ULL)));
    CHECK_EQ(8, bits_count(static_cast<unsigned char>(0xff)));
}

TEST_CASE("portable_bit_helpers_match_builtins") {
    // The fallbacks are what MSVC without AVX and other compilers use; check them on this compiler.
    bits64 x = 0x9e3779b97f4a7c15ULL;
    for (int t = 0; t != 1000; ++t) {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        bits64 v = x >> (t % 64);
        CHECK_EQ(details::portable_popcount(v), bits_count(v));
        if (v) {
            CHECK_EQ(details::portable_lowest(v), lowest_bit_index(v));
            CHECK_EQ(details::portable_highest(v), highest_bit_index(v));
        }
    }
    static_assert(details::portable_popcount(~bits64(0)) == 64, "constexpr popcount");
    static_assert(details::portable_lowest(bits64(1) << 63) == 63, "constexpr lowest bit");
    static_assert(details::portable_highest(1) == 0, "constexpr highest bit");
}

TEST_CASE("low_bit_iterator_64") {
    std::vector<int> i;
    for (low_bit<bits64> lb(0x8000000100000001ULL); lb; ++lb)
        i.push_back(lb);
    CHECK(i == std::vector<int>{0, 32, 63});
}
