/// @file
/// @brief `IntegersFileReader`: reading integer lists and bit matrices (`.hg` files).

#pragma once
#include <cstdio>

#include <dual/bit_matrix.h>
#include <dual/io_utils.h>

namespace dual {

/// @brief Reading integer data from a text file.
///
/// Numbers are runs of decimal digits; every other character (space, comma, ...) is a separator,
/// so signs are not recognised. Reading methods require an opened file.
class IntegersFileReader {
    FILE_ptr _input;

    bit_matrix _bm;

    int _lastChar;

private:
    bool IsDigitChar(int c) const {
        return '0' <= c && c <= '9';
    }

    int ReadUnsignedInteger(int firstDigit);

    int EstimateBitMatrixCapacity() const;

public:
    /// @brief Reader without a file; call `OpenInputFile` before reading.
    IntegersFileReader();

    /// @brief Constructor that opens the file.
    /// @throws std::runtime_error If the file cannot be opened.
    IntegersFileReader(std::string const& filename);

    /// @brief Open the file for reading.
    /// @throws std::runtime_error If the file cannot be opened.
    void OpenInputFile(std::string const& filename);

    /// @brief Close the file.
    void CloseInputFile();

    /// @brief Check for end of file.
    bool IsEOF() const;

    /// @brief Read a bit matrix from the rest of the file (`.hg` format).
    ///
    /// Every line is a row given by the 0-based indices of its columns that hold ones; an empty line
    /// is a zero row, trailing empty lines are not rows. The width is one past the largest index.
    /// @return The matrix; the reader's internal matrix is moved out.
    /// @throws std::out_of_range On a number that does not fit into `int` or is not a valid column
    ///         index (`INT_MAX`).
    /// @throws std::length_error If the matrix does not fit into `int` blocks.
    bit_matrix ReadBitMatrix();

    /// @brief Read a bit matrix with at most 32 columns from the rest of the file, one word per row.
    /// @param first Index of the first column (subtracted from every number read).
    /// @throws std::out_of_range If a column falls outside `[0, 32)`.
    /// @note Unlike `ReadBitMatrix`, a last line without `'\n'` becomes a row only if it is non-empty.
    std::vector<bits32> ReadBitMatrix32(int first = 0);

    /// @brief Read a bit matrix with at most 64 columns from the rest of the file, one word per row.
    /// @param first Index of the first column (subtracted from every number read).
    /// @throws std::out_of_range If a column falls outside `[0, 64)`.
    std::vector<bits64> ReadBitMatrix64(int first = 0);

    /// @brief Read the next integer from the file, skipping non-digit characters.
    /// @return The integer, or 0 at end of file.
    /// @throws std::out_of_range If the number does not fit into `int`.
    int ReadUnsignedInteger();

    /// @brief Read a sequence of integers from the file up to (and including) the delimiter.
    /// @param[out] integers Container the integers are appended to (`push_back`).
    /// @param delim Character that ends the sequence.
    /// @throws std::out_of_range If a number does not fit into `int`.
    template<typename Container>
    void ReadUnsignedIntegers(Container& integers, char delim = '\n') {
        assert(_input);
        while (!feof(_input.get())) {
            _lastChar = getc(_input.get());
            if (IsDigitChar(_lastChar))
                integers.push_back(ReadUnsignedInteger(_lastChar - '0'));

            if (_lastChar == delim)
                return;
        }
    }

    /// @brief Read a matrix (list of lists) of integers from the rest of the file, rows separated by `'\n'`.
    ///
    /// Rows are appended to @p integersList; trailing empty rows are dropped.
    /// @throws std::out_of_range If a number does not fit into `int`.
    template<typename Container>
    void ReadUnsignedIntegersMatrix(Container& integersList) {
        assert(_input);
        while (!feof(_input.get())) {
            integersList.resize(integersList.size() + 1);
            ReadUnsignedIntegers(integersList.back());
        }

        while (!integersList.empty() && integersList.back().empty())
            integersList.pop_back();
    }
};

}  // namespace dual
