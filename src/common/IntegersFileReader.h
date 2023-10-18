#pragma once
#include <stdio.h>

#include "bit_matrix.h"
#include "io_utils.h"

// ������ �� ����� ������������� ������
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

    // ����������� � ��������� �����
    IntegersFileReader(std::string const& filename);

    // ������� ���� ��� ������
    void OpenInputFile(std::string const& filename);

    // ������� ����
    void CloseInputFile();

    // �������� ����� �����
    bool IsEOF() const;

    // ��������� ������� ������� �� �����
    bit_matrix ReadBitMatrix();

    // ��������� ������� ������� �� �����
    std::vector<bits32> ReadBitMatrix32(int first = 0);

    // ��������� ������� ������� �� �����
    std::vector<bits64> ReadBitMatrix64(int first = 0);

    // ��������� ����� ����� �� �����
    int ReadUnsignedInteger();

    // ��������� �� ����� ����� ����� ����� �� �������� �� ��������� ������
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

    // ��������� �� ����� ������� (������ �������) ����� �����, ����������� �������� '\n'
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