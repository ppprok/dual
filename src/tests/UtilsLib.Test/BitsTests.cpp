#include <vector>

#include <doctest.h>

#include "bits.h"
#include "containers.h"

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
