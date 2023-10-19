#include "io_utils.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

std::shared_ptr<FILE> open_file_for_read(std::string const& filename) {
    FILE* f = fopen(filename.c_str(), "rb");

    if (!f)
        throw std::runtime_error("Open file error");

    return {f, fclose};
}

FILE_ptr create_file_for_write(std::string const& filename) {
    FILE* f = fopen(filename.c_str(), "wb");

    if (!f)
        throw std::runtime_error("Create file error");

    return {f, fclose};
}

std::string read_all(std::string const& filename) {
    auto size = std::filesystem::file_size(filename);
    std::string buffer(size, '\0');
    std::ifstream f(filename, std::ios::binary | std::ios::in);
    f.read(buffer.data(), size);
    return buffer;
}

void write(FILE* file, char const* begin, char const* end) {
    assert(file);
    auto count = end - begin;
    if (count == 0)
        return;

    if (fwrite(reinterpret_cast<void*>(const_cast<char*>(begin)), 1, count, file) != count) {
        throw std::runtime_error("Write to file error");
    }
}

void write(FILE* file, std::string const& str) {
    assert(file);
    auto count = str.length();
    if (count == 0)
        return;

    if (fwrite(reinterpret_cast<void*>(const_cast<char*>(&str[0])), 1, count, file) != count) {
        throw std::runtime_error("Write to file error");
    }
}

void write(FILE* file, char const* begin) {
    assert(file);
    int r = 0;
    for (; *begin && r != EOF; ++begin)
        r = fputc(*begin, file);
    if (r == EOF) {
        throw std::runtime_error("Write to file error");
    }
}
