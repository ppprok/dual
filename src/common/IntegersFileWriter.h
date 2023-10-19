#pragma once

#include "bit_matrix.h"
#include "io_utils.h"

typedef std::vector<int> Positions;

// ���������� ������ ����� ����� ����
class IntegersFileWriter {

    FILE_ptr _output;

public:
    IntegersFileWriter(std::string const& filename);

    IntegersFileWriter();

    // ������� ����
    void CloseOutputFile();

    // ������� ���� ��� ������
    void CreateOutputFile(std::string const& outputFileName);

    // ������
    void WriteChar(char c) {
        putc(c, _output.get());
    }

    // �����
    void WriteDigit(int digit) {
        WriteChar(digit + '0');
    }

    // ����� ������
    void WriteEOL() {
        WriteChar('\n');
    }

    // ������� ����� � ����
    void WriteInteger(int i, int pad = 0);

    void WriteNumber(double v, int precision = 3, int pad = 0);

    template<typename C>
    void WriteIntegers(C const& c) {
        WriteIntegers(c.begin(), c.end());
    }

    // ������� ������ ����� � ����
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

    // �������� ������� ����� �������� � ���� --- ������ ������ � ��������� ������
    // [beg, end) --- ������, ������ ������� �������� �������� �������� �����
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

    // �������� ������� ��������� �����
    void WriteBitNumbers(bit_chunk bits);

    // �������� ������ ������� � ���� ����������
    void WriteBitMatrix(bit_matrix const& bm);

    // �������� ������ ������� � ���� ����������
    void WriteBitMatrix(bit_chunks const& rows);
};
