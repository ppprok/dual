#pragma once
#include <stdio.h>

#include "bit_matrix.h"
#include "io_utils.h"

// Reading integer data from a file
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
    IntegersFileReader();

    // Constructor that opens the file
    IntegersFileReader(std::string const& filename);

    // Open the file for reading
    void OpenInputFile(std::string const& filename);

    // Close the file
    void CloseInputFile();

    // Check for end of file
    bool IsEOF() const;

    // Read a bit matrix from the file: every line is a row of column indices, an empty line is a zero row,
    // trailing empty lines are not rows. Throws std::out_of_range on a number that does not fit into int or
    // is not a valid column index (INT_MAX), and std::length_error when the matrix does not fit into int blocks
    bit_matrix ReadBitMatrix();

    // Read a bit matrix from the file
    std::vector<bits32> ReadBitMatrix32(int first = 0);

    // Read a bit matrix from the file
    std::vector<bits64> ReadBitMatrix64(int first = 0);

    // Read an integer from the file
    int ReadUnsignedInteger();

    // Read a sequence of integers from the file up to the end of the line
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

    // Read a matrix (list of lists) of integers from the file, rows separated by '\n'
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