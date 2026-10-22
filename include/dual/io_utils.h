/// @file
/// @brief `FILE`-based I/O helpers.

#pragma once

#include <memory>
#include <string>

#include <dual/blob.h>

/// `FILE` handle that is closed with `fclose` when the last owner releases it.
typedef std::shared_ptr<FILE> FILE_ptr;

/// @brief Open a file for binary reading.
/// @throws std::runtime_error If the file cannot be opened.
FILE_ptr open_file_for_read(std::string const& filename);

/// @brief Create (truncate) a file for binary writing.
/// @throws std::runtime_error If the file cannot be created.
FILE_ptr create_file_for_write(std::string const& filename);

/// @brief Read the whole file into a string.
/// @throws std::filesystem::filesystem_error If the file size cannot be determined.
std::string read_all(std::string const& filename);

/// @brief Write string to file.
/// @throws std::runtime_error On a write error.
void write(FILE* file, std::string const& str);

/// @brief Write the characters between @p begin and @p end to file.
/// @throws std::runtime_error On a write error.
void write(FILE* file, char const* begin, char const* end);

/// @brief Write a zero-terminated string to file.
/// @throws std::runtime_error On a write error.
void write(FILE* file, char const* begin);

/// @brief Write the raw bytes of the objects `[begin, end)` to file.
/// @throws std::runtime_error On a write error.
template<typename T>
void write(FILE* file, T* begin, T* end) {
    write(file, reinterpret_cast<char const*>(begin), reinterpret_cast<char const*>(end));
}

/// @brief Write the raw bytes of every element of `[begin, end)` to file, one element at a time.
/// @throws std::runtime_error On a write error.
template<typename Iter>
void write(FILE* file, Iter begin, Iter end) {
    if (begin == end)
        return;

    for (; begin != end; ++begin) {
        auto ptr = &*begin;
        write(file, ptr, ptr + 1);
    }
}

/// @brief Create file @p filename and write the raw bytes of `[beg, end)` to it.
/// @throws std::runtime_error If the file cannot be created or written.
template<typename T>
void write_all(std::string const& filename, T* beg, T* end) {
    auto file = create_file_for_write(filename);
    write(file.get(), beg, end);
}

/// @brief Create file @p filename and write the raw bytes of a contiguous container to it.
/// @throws std::runtime_error If the file cannot be created or written.
template<typename Cont>
void write_all(std::string const& filename, Cont container) {
    auto file = create_file_for_write(filename);
    if (!container.empty())
        write(file.get(), &container.front(), (&container.back()) + 1);
}
