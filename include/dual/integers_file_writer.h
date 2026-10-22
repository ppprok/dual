/// @file
/// @brief `IntegersFileWriter`: writing integer lists and bit matrices as text.

#pragma once

#include <dual/bit_matrix.h>
#include <dual/io_utils.h>

/// List of positions (e.g. column indices).
typedef std::vector<int> Positions;

/// @brief Writing integers to a text file.
///
/// Writing methods require an opened file (the constructor with a name or `CreateOutputFile`).
class IntegersFileWriter {

    FILE_ptr _output;

public:
    /// @brief Writer that creates the file @p filename.
    /// @throws std::runtime_error If the file cannot be created.
    IntegersFileWriter(std::string const& filename);

    /// @brief Writer without a file; call `CreateOutputFile` before writing.
    IntegersFileWriter();

    /// @brief Close the file.
    void CloseOutputFile();

    /// @brief Create the file for writing (truncating an existing one).
    /// @throws std::runtime_error If the file cannot be created.
    void CreateOutputFile(std::string const& outputFileName);

    /// @brief Write a character.
    void WriteChar(char c) {
        putc(c, _output.get());
    }

    /// @brief Write a decimal digit (`0 <= digit <= 9`).
    void WriteDigit(int digit) {
        WriteChar(digit + '0');
    }

    /// @brief Write an end of line (`'\n'`).
    void WriteEOL() {
        WriteChar('\n');
    }

    /// @brief Write an integer to the file in decimal.
    /// @param i Integer to write.
    /// @param pad Minimum number of digits; shorter numbers are padded with leading zeros.
    void WriteInteger(int i, int pad = 0);

    /// @brief Write a real number in fixed-point notation.
    /// @param v Number to write.
    /// @param precision Number of digits after the point; the fraction is truncated and omitted
    ///        for whole numbers.
    /// @param pad Minimum number of digits of the integer part.
    void WriteNumber(double v, int precision = 3, int pad = 0);

    /// @brief Write the integers of a container separated by spaces.
    template<typename C>
    void WriteIntegers(C const& c) {
        WriteIntegers(c.begin(), c.end());
    }

    /// @brief Write a list of numbers `[beg, end)` to the file, separated by spaces.
    template<typename Iter>
    void WriteIntegers(Iter beg, Iter end) {
        if (beg == end)
            return;

        for (;;) {
            WriteInteger(*beg);
            if (++beg == end)
                break;
            WriteChar(' ');
        }
    }

    /// @brief Write a container of integer arrays, one array per line (see the iterator overload).
    template<typename Matrix>
    void WriteIntegerMatrix(Matrix const& c) {
        WriteIntegerMatrix(c.begin(), c.end());
    }

    /// @brief Write a set of arrays to the file, each array on a separate line.
    ///
    /// `[beg, end)` is a list whose elements are arrays of integers. No end of line is written
    /// after the last array.
    template<typename Iter>
    void WriteIntegerMatrix(Iter beg, Iter end) {
        if (beg == end)
            return;

        for (;;) {
            WriteIntegers(std::begin(*beg), std::end(*beg));
            if (++beg == end)
                break;
            WriteEOL();
        }
    }

    /// @brief Write the positions of the set bits, separated by spaces.
    void WriteBitNumbers(bit_chunk bits);

    /// @brief Write a Boolean matrix in hypergraph (`.hg`) format: one line per row with the
    ///        indices of its ones; no end of line after the last row.
    void WriteBitMatrix(bit_matrix const& bm);

    /// @brief Write rows in hypergraph (`.hg`) format (see `WriteBitMatrix(bit_matrix const&)`).
    void WriteBitMatrix(bit_chunks const& rows);
};
