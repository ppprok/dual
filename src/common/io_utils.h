#pragma once

#include <memory>
#include <string>

#include "blob.h"
#include "errors.h"

typedef std::shared_ptr<FILE> FILE_ptr;


struct file_error : virtual error { };
struct file_open_error: virtual file_error { };
struct file_create_error: virtual file_error { };
struct file_read_error: virtual file_error { };
struct file_write_error: virtual file_error { };


FILE_ptr open_file_for_read(std::string const& filename);

FILE_ptr create_file_for_write(std::string const& filename);

std::string read_all(std::string const &filename);

/*!
    Write string to file
*/
void write(FILE* file, std::string const& str);

/*!
    Write string between begin and end to file
*/
void write(FILE* file, char const* begin, char const* end);

/*!
    Write zero-ended string to file
*/
void write( FILE* file, char const* begin );

template<typename T>
void write(FILE* file, T* begin, T* end)
{
    write(file, 
        reinterpret_cast<char const*>(begin), 
        reinterpret_cast<char const*>(end));
}


template<typename Iter>
void write(FILE* file, Iter begin, Iter end)
{
    if (begin == end)
        return;
    
    for(; begin != end; ++begin)
    {
        auto ptr = &*begin;
        write(file, ptr, ptr+1);
    }
}

template <typename T>
void write_all(std::string const& filename, T* beg, T* end)
{
    auto file = create_file_for_write(filename);
    write(file.get(), beg, end);
}

template <typename Cont>
void write_all(std::string const& filename, Cont container)
{
    auto file = create_file_for_write(filename);
    if (! container.empty())
        write(file.get(), &container.front(), (&container.back())+1);
}