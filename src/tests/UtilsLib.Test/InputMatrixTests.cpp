// Regression tests for #22: silent corruption of the input matrix in bit_matrix::set and IntegersFileReader.
#include <doctest.h>

#include <algorithm>
#include <climits>
#include <cstdio>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <dual/bit_matrix.h>
#include <dual/integers_file_reader.h>

using namespace dual;

namespace {

// Temporary file with the given text, unique per process and test run, removed in the destructor
class TempFile {
    std::string _path;

public:
    TempFile(std::string const& name, std::string const& text) {
        static std::mt19937_64 rnd{std::random_device{}()};
        auto unique = "dual_test22_" + std::to_string(rnd()) + "_" + name;
        _path = (std::filesystem::temp_directory_path() / unique).string();
        FILE* f = fopen(_path.c_str(), "wb");
        REQUIRE(f);
        fwrite(text.data(), 1, text.size(), f);
        fclose(f);
    }

    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(_path, ec);
    }

    TempFile(TempFile const&) = delete;
    TempFile& operator=(TempFile const&) = delete;

    std::string const& path() const {
        return _path;
    }
};

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
    TempFile file("empty_first_line.hg", "\n0 1\n1 2\n");
    IntegersFileReader reader(file.path());
    bit_matrix bm = reader.ReadBitMatrix();

    // Every line is a row, so the empty first line is a zero row
    REQUIRE(bm.height() == 3);
    CheckRow(bm, 0, {});
    CheckRow(bm, 1, {0, 1});
    CheckRow(bm, 2, {1, 2});
}

TEST_CASE("read_bit_matrix_empty_middle_line") {
    TempFile file("empty_middle_line.hg", "0 1\n\n1 2\n\n");
    IntegersFileReader reader(file.path());
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
    TempFile file("capacity.hg", text);
    IntegersFileReader reader(file.path());
    bit_matrix bm = reader.ReadBitMatrix();

    REQUIRE(bm.height() == m);
    for (int i = 0; i < m; ++i)
        CHECK(bm.get(i, 10 + i % 22));
    // The whole matrix is reserved once, from the size of the first two rows and the file size
    // (growing by 5/4 steps would end up to a quarter above the size)
    CHECK(bm.size() == m);
    CHECK(bm.capacity() >= m);
    CHECK(bm.capacity() <= m + m / 50);
}

// Numbers that do not fit into int must not be silently wrapped
TEST_CASE("read_integer_overflow") {
    SUBCASE("bit matrix") {
        TempFile file("overflow.hg", "0 1\n2 99999999999\n");
        IntegersFileReader reader(file.path());
        CHECK_THROWS_AS(reader.ReadBitMatrix(), std::out_of_range);
    }
    SUBCASE("integers") {
        TempFile file("overflow.txt", "1 2147483648\n");
        IntegersFileReader reader(file.path());
        std::vector<int> result;
        CHECK_THROWS_AS(reader.ReadUnsignedIntegers(result), std::out_of_range);
    }
    SUBCASE("int max") {
        TempFile file("int_max.txt", "2147483647 7\n");
        IntegersFileReader reader(file.path());
        std::vector<int> result;
        reader.ReadUnsignedIntegers(result);
        CHECK(result == std::vector<int>{2147483647, 7});
    }
}

// Review of #22: setting a bit in the last row below its end must not shrink the buffer under the row
TEST_CASE("bit_matrix_set_last_row_keeps_buffer") {
    bit_matrix bm;
    CHECK(bm.set(5, 70));
    CHECK(bm.set(5, 0));

    REQUIRE(bm.height() == 6);
    CHECK(bm.size() <= bm.capacity());
    CHECK(bm.get(5, 70));
    CheckRow(bm, 5, {0, 70});
}

// Column and row INT_MAX would overflow j + 1 and i + 1, so set rejects them
TEST_CASE("bit_matrix_set_int_max") {
    bit_matrix bm;
    CHECK_FALSE(bm.set(0, INT_MAX));
    CHECK_FALSE(bm.set(INT_MAX, 0));
    CHECK(bm.height() == 0);
    CHECK(bm.width() == 0);

    CHECK(bm.set(0, 1));
    CHECK_FALSE(bm.set(0, INT_MAX));
    CHECK(bm.width() == 2);
    CheckRow(bm, 0, {1});
}

TEST_CASE("read_bit_matrix_int_max") {
    TempFile file("int_max.hg", "0 1\n2 2147483647\n");
    IntegersFileReader reader(file.path());
    CHECK_THROWS_AS(reader.ReadBitMatrix(), std::out_of_range);
}

// Block counts that do not fit into int throw before anything is allocated instead of wrapping around
TEST_CASE("bit_matrix_too_many_blocks") {
    SUBCASE("new rows") {
        // 41 rows of 62500001 blocks each
        bit_matrix bm;
        CHECK_THROWS_AS(bm.set(40, 2000000000), std::length_error);
        CHECK(bm.height() == 0);
        CHECK(bm.width() == 0);
    }
    SUBCASE("restride") {
        bit_matrix bm;
        CHECK(bm.set(40, 0));
        CHECK_THROWS_AS(bm.set(0, 2000000000), std::length_error);
        REQUIRE(bm.height() == 41);
        CHECK(bm.width() == 1);
        CHECK(bm.get(40, 0));
    }
    SUBCASE("reserve") {
        bit_matrix bm;
        CHECK_THROWS_AS(bm.reserve(41, 2000000000), std::length_error);
    }
}
