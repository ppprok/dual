#include "IntegersFileReader.h"
#include "errors.h"

bit_matrix IntegersFileReader::ReadBitMatrix() {
    assert(_input);
    int i = 0;
    while (!feof(_input.get())) {
        _lastChar = getc(_input.get());
        if (IsDigitChar(_lastChar)) {
            int j = ReadUnsignedInteger(_lastChar - '0');
            _bm.set(i, j);
        }

        if (_lastChar == '\n') {
            if (i == 1)
                _bm.reserve(EstimateBitMatrixCapacity());
            ++i;
        }
    }

    return std::move(_bm);
}

IntegersFileReader::IntegersFileReader() {}

IntegersFileReader::IntegersFileReader(std::string const& filename) {
    OpenInputFile(filename);
}

int IntegersFileReader::EstimateBitMatrixCapacity() const {
    int size = _bm.size();
    long fsize = 0;
    long pos = ftell(_input.get());
    return size * (fsize / pos + 1);
}

void IntegersFileReader::OpenInputFile(std::string const& filename) {
    _input = open_file_for_read(filename);
}

int IntegersFileReader::ReadUnsignedInteger() {
    assert(_input);
    while (!feof(_input.get())) {
        _lastChar = getc(_input.get());
        if (IsDigitChar(_lastChar)) {
            return ReadUnsignedInteger(_lastChar - '0');
        }
    }

    return 0;
}

int IntegersFileReader::ReadUnsignedInteger(int firstDigit) {
    int v = firstDigit;
    while (!feof(_input.get())) {
        _lastChar = getc(_input.get());
        if (!IsDigitChar(_lastChar))
            return v;
        v = v * 10 + (_lastChar - '0');
    }
    return v;
}

bool IntegersFileReader::IsEOF() const {
    return feof(_input.get()) != 0;
}

void IntegersFileReader::CloseInputFile() {
    _input.reset();
}

std::vector<bits32> IntegersFileReader::ReadBitMatrix32(int first) {
    assert(_input);
    int i = 0;

    std::vector<bits32> bm;
    bits32 row = 0;

    while (!feof(_input.get())) {
        _lastChar = getc(_input.get());
        if (IsDigitChar(_lastChar)) {
            int j = ReadUnsignedInteger(_lastChar - '0') - first;
            if (j < 0 || j > 31)
                throw std::out_of_range("input more than 32 bits");
            row |= (bits32) 1 << j;
        }

        if (_lastChar == '\n') {
            bm.push_back(row);
            row = 0;
        }
    }

    if (row)
        bm.push_back(row);

    return std::move(bm);
}

std::vector<bits64> IntegersFileReader::ReadBitMatrix64(int first) {
    assert(_input);
    int i = 0;

    std::vector<bits64> bm;
    bits64 row = 0;

    while (!feof(_input.get())) {
        _lastChar = getc(_input.get());
        if (IsDigitChar(_lastChar)) {
            int j = ReadUnsignedInteger(_lastChar - '0') - first;
            if (j < 0 || j > 63)
                throw std::out_of_range("input more than 64 bits");
            row |= ((bits64) 1) << j;
        }

        if (_lastChar == '\n') {
            bm.push_back(row);
            row = 0;
        }
    }

    if (row)
        bm.push_back(row);

    return std::move(bm);
}