#include <doctest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <dual/integers_file_reader.h>
#include <dual/integers_file_writer.h>
#include <dual/io_utils.h>

using namespace dual;

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

    for (const auto& v : hg) {
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

// #47: the file helpers of io_utils.h and the error paths of the integer reader and writer
TEST_CASE("io_utils: read and write whole files, errors on missing files") {
    std::filesystem::create_directories("temp");
    CHECK_THROWS_AS(open_file_for_read("temp/no_such_file.txt"), std::runtime_error);
    CHECK_THROWS_AS(create_file_for_write("temp/no_such_dir/file.txt"), std::runtime_error);
    CHECK_THROWS_AS(IntegersFileReader("temp/no_such_file.txt"), std::runtime_error);

    std::vector<int> ints = {1, -2, 3};
    write_all("temp/io_ints.bin", ints);
    CHECK_EQ(read_all("temp/io_ints.bin").size(), 3 * sizeof(int));
    write_all("temp/io_ints.bin", std::vector<int>());
    CHECK(read_all("temp/io_ints.bin").empty());
    write_all("temp/io_ints.bin", ints.data(), ints.data() + 2);
    CHECK_EQ(read_all("temp/io_ints.bin").size(), 2 * sizeof(int));

    {
        auto file = create_file_for_write("temp/io_text.txt");
        write(file.get(), std::string("ab"));
        write(file.get(), std::string());
        char const text[] = "cd";
        write(file.get(), text, text + 2);
        write(file.get(), text, text);
        write(file.get(), "ef");
        std::vector<char> chars = {'g', 'h'};
        write(file.get(), chars.begin(), chars.end());
        write(file.get(), chars.end(), chars.end());
    }
    CHECK_EQ(read_all("temp/io_text.txt"), "abcdefgh");
}

TEST_CASE("IntegersFileWriter: signs, padding and bit rows") {
    std::filesystem::create_directories("temp");
    {
        IntegersFileWriter writer;
        writer.CreateOutputFile("temp/io_writer.txt");
        writer.WriteInteger(-42);
        writer.WriteChar(' ');
        writer.WriteInteger(7, 3);
        writer.WriteChar(' ');
        writer.WriteInteger(42, 4);
        writer.WriteChar(' ');
        writer.WriteInteger(12345, 7);
        writer.WriteChar(' ');
        writer.WriteNumber(-2.25, 2);
        writer.WriteChar(' ');
        writer.WriteNumber(3.5, 1, 3);
        writer.WriteEOL();
        writer.WriteIntegerMatrix(std::vector<std::vector<int>>{{1, 2}, {}, {3}});
        writer.WriteEOL();
        writer.WriteIntegerMatrix(std::vector<std::vector<int>>());
        bit_matrix bm;
        bm.set(0, 3);
        bm.set(0, 70);
        bm.set(2, 1);
        writer.WriteBitMatrix(bm.get_rows());
        writer.WriteEOL();
        writer.WriteBitMatrix(bit_chunks());
        writer.WriteBitMatrix(bit_matrix());
        writer.CloseOutputFile();
    }
    CHECK_EQ(read_all("temp/io_writer.txt"), "-42 007 0042 0012345 -2.25 003.5\n1 2\n\n3\n3 70\n\n1\n");
}

TEST_CASE("IntegersFileReader: incremental reading and malformed input") {
    std::filesystem::create_directories("temp");
    write_all("temp/io_reader.txt", std::string("5 17\n3"));
    IntegersFileReader reader;
    reader.OpenInputFile("temp/io_reader.txt");
    CHECK_FALSE(reader.IsEOF());
    CHECK_EQ(5, reader.ReadUnsignedInteger());
    CHECK_EQ(17, reader.ReadUnsignedInteger());
    CHECK_EQ(3, reader.ReadUnsignedInteger());
    CHECK_EQ(0, reader.ReadUnsignedInteger());  // nothing left
    CHECK(reader.IsEOF());
    reader.CloseInputFile();

    // A last line without '\n' is a row of the bit masks
    CHECK(IntegersFileReader("temp/io_reader.txt").ReadBitMatrix32()
          == std::vector<bits32>{(1u << 5) | (1u << 17), 8u});
    CHECK(IntegersFileReader("temp/io_reader.txt").ReadBitMatrix64(3)
          == std::vector<bits64>{(1ull << 2) | (1ull << 14), 1ull});
    // Columns outside the mask width or below `first`
    CHECK_THROWS_AS(IntegersFileReader("temp/io_reader.txt").ReadBitMatrix32(4), std::out_of_range);
    CHECK_THROWS_AS(IntegersFileReader("temp/io_reader.txt").ReadBitMatrix64(4), std::out_of_range);
    write_all("temp/io_reader.txt", std::string("1 32\n"));
    CHECK_THROWS_AS(IntegersFileReader("temp/io_reader.txt").ReadBitMatrix32(), std::out_of_range);
    write_all("temp/io_reader.txt", std::string("64\n"));
    CHECK_THROWS_AS(IntegersFileReader("temp/io_reader.txt").ReadBitMatrix64(), std::out_of_range);
    write_all("temp/io_reader.txt", std::string("99999999999\n"));
    CHECK_THROWS_AS(IntegersFileReader("temp/io_reader.txt").ReadUnsignedInteger(), std::out_of_range);
}
