// Regression tests for #22: silent corruption of the input matrix in bit_matrix::set and IntegersFileReader.
#include <doctest.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "IntegersFileReader.h"
#include "common/bit_matrix.h"

namespace {

// Write text to a temporary file and return its path
std::string WriteTempFile(std::string const& name, std::string const& text) {
    auto path = (std::filesystem::temp_directory_path() / ("dual_test22_" + name)).string();
    FILE* f = fopen(path.c_str(), "wb");
    REQUIRE(f);
    fwrite(text.data(), 1, text.size(), f);
    fclose(f);
    return path;
}

// Check that row i of the matrix contains exactly the given columns
void CheckRow(bit_matrix const& bm, int i, std::vector<int> const& cols) {
    CAPTURE(i);
    for (int j : cols)
        CHECK(bm.get(i, j));
    CHECK(bm.get_row(i).count() == (int) cols.size());
}

}  // namespace

// Finding 6: the first bit set with row_num > 0 went to row 0
TEST_CASE("bit_matrix_set_first_row_not_zero") {
    bit_matrix bm;
    CHECK(bm.set(1, 0));
    CHECK(bm.set(0, 1));

    REQUIRE(bm.height() == 2);
    CHECK(bm.width() == 2);
    CheckRow(bm, 0, {1});
    CheckRow(bm, 1, {0});
}

// Finding 8: a bit past the stride of a non-last row was silently dropped
TEST_CASE("bit_matrix_set_past_row_stride") {
    bit_matrix bm;
    CHECK(bm.set(0, 0));
    CHECK(bm.set(1, 1));
    CHECK(bm.set(0, 40));

    REQUIRE(bm.height() == 2);
    CHECK(bm.width() == 41);
    CheckRow(bm, 0, {0, 40});
    CheckRow(bm, 1, {1});

    // The matrix keeps growing correctly after re-striding
    CHECK(bm.set(1, 100));
    CHECK(bm.set(2, 70));
    CHECK(bm.set(0, 200));
    REQUIRE(bm.height() == 3);
    CHECK(bm.width() == 201);
    CheckRow(bm, 0, {0, 40, 200});
    CheckRow(bm, 1, {1, 100});
    CheckRow(bm, 2, {70});
}

// Rows and columns may be set in any order, as the C ABI set_input_row allows
TEST_CASE("bit_matrix_set_any_order") {
    std::mt19937 rnd(22);
    for (int iter = 0; iter < 50; ++iter) {
        int m = 1 + rnd() % 40;
        int n = 1 + rnd() % 300;
        std::vector<std::vector<int>> etalon(m);
        std::vector<std::pair<int, int>> bits;
        for (int i = 0; i < m; ++i)
            for (int j = 0; j < n; ++j)
                if (rnd() % 4 == 0) {
                    etalon[i].push_back(j);
                    bits.emplace_back(i, j);
                }
        std::shuffle(bits.begin(), bits.end(), rnd);

        bit_matrix bm;
        for (auto const& b : bits)
            CHECK(bm.set(b.first, b.second));

        CAPTURE(iter);
        int lastRow = m - 1;
        while (lastRow >= 0 && etalon[lastRow].empty())
            --lastRow;
        REQUIRE(bm.height() == lastRow + 1);
        CHECK(bm.size() <= bm.capacity());
        for (int i = 0; i < bm.height(); ++i)
            CheckRow(bm, i, etalon[i]);
    }
}

// Finding 7: an empty first line split the next line into two rows
TEST_CASE("read_bit_matrix_empty_first_line") {
    IntegersFileReader reader(WriteTempFile("empty_first_line.hg", "\n0 1\n1 2\n"));
    bit_matrix bm = reader.ReadBitMatrix();

    // Every line is a row, so the empty first line is a zero row
    REQUIRE(bm.height() == 3);
    CheckRow(bm, 0, {});
    CheckRow(bm, 1, {0, 1});
    CheckRow(bm, 2, {1, 2});
}

TEST_CASE("read_bit_matrix_empty_middle_line") {
    IntegersFileReader reader(WriteTempFile("empty_middle_line.hg", "0 1\n\n1 2\n\n"));
    bit_matrix bm = reader.ReadBitMatrix();

    // A zero row in the middle is kept, trailing empty lines are not rows
    REQUIRE(bm.height() == 3);
    CheckRow(bm, 0, {0, 1});
    CheckRow(bm, 1, {});
    CheckRow(bm, 2, {1, 2});
}

// Finding 17: EstimateBitMatrixCapacity ignored the file size, so the matrix grew by small steps
TEST_CASE("read_bit_matrix_capacity_estimate") {
    const int m = 1000;
    std::string text;
    for (int i = 0; i < m; ++i)
        text += "0 1 " + std::to_string(10 + i % 22) + "\n";  // lines of equal length
    IntegersFileReader reader(WriteTempFile("capacity.hg", text));
    bit_matrix bm = reader.ReadBitMatrix();

    REQUIRE(bm.height() == m);
    for (int i = 0; i < m; ++i)
        CHECK(bm.get(i, 10 + i % 22));
    // The whole matrix is reserved once, from the size of the first two rows and the file size
    CHECK(bm.size() == m);
    CHECK(bm.capacity() == m + 2);
}

// Numbers that do not fit into int must not be silently wrapped
TEST_CASE("read_integer_overflow") {
    SUBCASE("bit matrix") {
        IntegersFileReader reader(WriteTempFile("overflow.hg", "0 1\n2 99999999999\n"));
        CHECK_THROWS_AS(reader.ReadBitMatrix(), std::out_of_range);
    }
    SUBCASE("integers") {
        IntegersFileReader reader(WriteTempFile("overflow.txt", "1 2147483648\n"));
        std::vector<int> result;
        CHECK_THROWS_AS(reader.ReadUnsignedIntegers(result), std::out_of_range);
    }
    SUBCASE("int max") {
        IntegersFileReader reader(WriteTempFile("int_max.txt", "2147483647 7\n"));
        std::vector<int> result;
        reader.ReadUnsignedIntegers(result);
        CHECK(result == std::vector<int>{2147483647, 7});
    }
}
