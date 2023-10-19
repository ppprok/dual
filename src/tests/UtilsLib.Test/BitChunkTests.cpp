#include <doctest.h>

#include "bit_mask.h"
#include "bit_vector.h"
#include "containers.h"

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

// Проверка перечисления различающихся битов двух
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
