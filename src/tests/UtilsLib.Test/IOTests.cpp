#include <doctest.h>
#include <filesystem>

#include "IntegersFileReader.h"
#include "common/IntegersFileWriter.h"

TEST_CASE("write_interger_test") {
    IntegersFileWriter writer("data/integers.txt");
    std::vector<int> integers = {100, 100000500, 190051001};
    writer.WriteIntegers(integers);
    writer.CloseOutputFile();

    IntegersFileReader reader("data/integers.txt");
    std::vector<int> result;
    reader.ReadUnsignedIntegers(result);
    CHECK(integers == result);
}

TEST_CASE("read_hypergraph_test") {
    IntegersFileReader reader("data/6_6_6.hg");

    std::vector<std::vector<int>> hg;

    reader.ReadUnsignedIntegersMatrix(hg);

    CHECK(hg.size() == 6);

    std::vector<std::vector<int>> etalon;
    etalon.push_back({0, 1, 3, 4, 5});
    etalon.push_back({0, 1, 2, 3, 4, 5});
    etalon.push_back({1, 3});
    etalon.push_back({3, 4});
    etalon.push_back({0, 1, 2, 4, 5});
    etalon.push_back({2, 3});

    CHECK(hg == etalon);
}

TEST_CASE("read_iris_test") {
    IntegersFileReader reader("data/iris.txt");

    std::vector<std::vector<int>> hg;

    reader.ReadUnsignedIntegersMatrix(hg);

    CHECK(hg.size() == 77);
    hg.pop_back();

    for (auto v : hg) {
        CHECK(v.size() == 5);
    }
}

TEST_CASE("read_bit_matrix") {
    IntegersFileReader reader("data/6_6_6.hg");

    bit_matrix bm = reader.ReadBitMatrix();

    CHECK(bm.height() == 6);
    CHECK(bm.width() == 6);

    std::vector<std::vector<int>> etalon;
    etalon.push_back({0, 1, 3, 4, 5});
    etalon.push_back({0, 1, 2, 3, 4, 5});
    etalon.push_back({1, 3});
    etalon.push_back({3, 4});
    etalon.push_back({0, 1, 2, 4, 5});
    etalon.push_back({2, 3});

    for (int i = 0; i < 6; ++i) {
        for (int j : etalon[i]) {
            CHECK(bm.get(i, j));
        }
        CHECK(bm.get_row(i).count() == etalon[i].size());
    }
}

TEST_CASE("write_bit_matrix") {
    bit_matrix bm;

    bm.set(0, 1);
    bm.set(0, 10);
    bm.set(0, 100);

    bm.set(1, 11);
    bm.set(1, 7);
    bm.set(1, 10);

    bm.set(2, 110);
    bm.set(2, 76);
    bm.set(2, 50);
    bm.set(2, 5);

    bm.set(3, 15);
    bm.set(3, 95);

    CHECK(bm.height() == 4);
    CHECK(bm.width() == 111);

    std::filesystem::create_directories("temp");

    IntegersFileWriter writer("temp/write_bm_test.hg");
    writer.WriteBitMatrix(bm);
    writer.CloseOutputFile();

    IntegersFileReader reader("temp/write_bm_test.hg");

    std::vector<std::vector<int>> hg;
    reader.ReadUnsignedIntegersMatrix(hg);

    std::vector<std::vector<int>> etalon;
    etalon.push_back({1, 10, 100});
    etalon.push_back({7, 10, 11});
    etalon.push_back({5, 50, 76, 110});
    etalon.push_back({15, 95});

    CHECK(hg == etalon);
}
