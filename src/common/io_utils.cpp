
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/string_file.hpp>
#include <unistd.h>

#include "errors.h"
#include "io_utils.h"

std::shared_ptr<FILE> open_file_for_read(std::string const& filename) {
    FILE* f = fopen(filename.c_str(), "rb");

    if (!f)
        BOOST_THROW_EXCEPTION(file_open_error()
                              << boost::errinfo_api_function("fopen") << boost::errinfo_errno(errno)
                              << boost::errinfo_file_name(filename) << boost::errinfo_file_open_mode("rb"));

    return std::shared_ptr<FILE>(f, fclose);
}

FILE_ptr create_file_for_write(std::string const& filename) {
    FILE* f = fopen(filename.c_str(), "wb");

    if (!f)
        BOOST_THROW_EXCEPTION(file_create_error()
                              << boost::errinfo_api_function("fopen") << boost::errinfo_errno(errno)
                              << boost::errinfo_file_name(filename) << boost::errinfo_file_open_mode("wb"));

    return std::shared_ptr<FILE>(f, fclose);
}

std::string read_all(std::string const& filename) {
    std::string buffer;
    boost::filesystem::load_string_file(filename, buffer);
    return std::move(buffer);
}

void write(FILE* file, char const* begin, char const* end) {
    assert(file);
    auto count = end - begin;
    if (count == 0)
        return;

    if (fwrite(reinterpret_cast<void*>(const_cast<char*>(begin)), 1, count, file) != count) {
        BOOST_THROW_EXCEPTION(file_write_error()
                              << boost::errinfo_api_function("fwrite") << boost::errinfo_errno(errno));
    }
}

void write(FILE* file, std::string const& str) {
    assert(file);
    auto count = str.length();
    if (count == 0)
        return;

    if (fwrite(reinterpret_cast<void*>(const_cast<char*>(&str[0])), 1, count, file) != count) {
        BOOST_THROW_EXCEPTION(file_write_error()
                              << boost::errinfo_api_function("fwrite") << boost::errinfo_errno(errno));
    }
}

void write(FILE* file, char const* begin) {
    assert(file);
    int r = 0;
    for (; *begin && r != EOF; ++begin)
        r = fputc(*begin, file);
    if (r == EOF) {
        BOOST_THROW_EXCEPTION(file_write_error()
                              << boost::errinfo_api_function("fputc") << boost::errinfo_errno(errno));
    }
}
