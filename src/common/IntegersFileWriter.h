#pragma once

#include "bit_matrix.h"
#include "io_utils.h"

typedef std::vector<int> Positions;

// Writing integers to a file
class IntegersFileWriter {

    FILE_ptr _output;

public:
    IntegersFileWriter(std::string const& filename);

    IntegersFileWriter();

    // Close the file
    void CloseOutputFile();

    // Create the file for writing
    void CreateOutputFile(std::string const& outputFileName);

    // Character
    void WriteChar(char c) {
        putc(c, _output.get());
    }

    // Digit
    void WriteDigit(int digit) {
        WriteChar(digit + '0');
    }

    // End of line
    void WriteEOL() {
        WriteChar('\n');
    }

    // Write a number to the file
    void WriteInteger(int i, int pad = 0);

    void WriteNumber(double v, int precision = 3, int pad = 0);

    template<typename C>
    void WriteIntegers(C const& c) {
        WriteIntegers(c.begin(), c.end());
    }

    // Write a list of numbers to the file
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

    template<typename Matrix>
    void WriteIntegerMatrix(Matrix const& c) {
        WriteIntegerMatrix(c.begin(), c.end());
    }

    // Write a set of arrays to the file --- each array on a separate line
    // [beg, end) --- a list whose elements are arrays of integers
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

    // Write the positions of the set bits
    void WriteBitNumbers(bit_chunk bits);

    // Write a Boolean matrix in hypergraph format
    void WriteBitMatrix(bit_matrix const& bm);

    // Write a Boolean matrix in hypergraph format
    void WriteBitMatrix(bit_chunks const& rows);
};
