/// @file
/// @brief String helpers.

#pragma once

#include <string>

#include <dual/containers.h>

namespace dual {

/// @brief C string of a C string (identity), for generic code.
inline char const* c_str(char const* c) {
    return c;
}

/// @brief C string of a `std::string`.
inline char const* c_str(std::string const& s) {
    return s.c_str();
}

/*
inline std::string unescape(std::string const& s)
{
    std::string out;
    out.reserve(s.length());
    
    for (auto b = s.begin(), e = s.end(); b!= e; ++b )
    {
        if (*b != '\\')
        {
            out.push_back(*b);
            continue;
        }

        ++b;
        if (b != e)
        {
            switch(*b)
            {
            case 't':
                out.push_back('\t');
                continue;
            case 'r':
                out.push_back('\r');
                continue;
            case 'n':
                out.push_back('\n');
                continue;
            case '\\':
                out.push_back('\\');
                continue;
            }            
        }                
        Throw("Unescape string error: " + std::string(s.begin(), b-1) + "^^^" + std::string(b, e));
        
    }

    return std::move(out);
}*/

/// Vector of strings.
typedef std::vector<std::string> Strings;

/// @brief Check whether the characters `[begin1, end1)` equal the zero-terminated string @p begin2.
inline bool streq(char const* begin1, char const* end1, char const* begin2) {
    int n = end1 - begin1;
    return n == strlen(begin2) && strncmp(begin1, begin2, n) == 0;
}

/// @brief Compare the character ranges `[begin1, end1)` and `[begin2, end2)`.
inline bool streq(char const* begin1, char const* end1, char const* begin2, char const* end2) {
    int n = end1 - begin1;
    return n == end2 - begin2 && strncmp(begin1, begin2, n) == 0;
}

/// @brief Check whether two zero-terminated strings are equal.
inline bool streq(char const* begin1, char const* begin2) {
    return strcmp(begin1, begin2) == 0;
}

}  // namespace dual
