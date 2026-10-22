#include <dual/integers_file_reader.h>

#include <algorithm>
#include <climits>
#include <stdexcept>

bit_matrix IntegersFileReader::ReadBitMatrix() {
    assert(_input);
    int i = 0;
    while (!feof(_input.get())) {
        _lastChar = getc(_input.get());
        if (IsDigitChar(_lastChar)) {
            int j = ReadUnsignedInteger(_lastChar - '0');
            if (!_bm.set(i, j))
                throw std::out_of_range("row or column index in the input file is too large");
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
    FILE* f = _input.get();
    long pos = ftell(f);
    if (pos <= 0 || fseek(f, 0, SEEK_END) != 0)
        return size;
    long fsize = ftell(f);
    if (fseek(f, pos, SEEK_SET) != 0)
        throw std::runtime_error("cannot restore the input file position");
    if (fsize <= pos)
        return size;
    return (int) std::min<long long>((long long) size * (fsize / pos + 1), INT_MAX);
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
        int d = _lastChar - '0';
        if (v > (INT_MAX - d) / 10)
            throw std::out_of_range("integer in the input file is too large");
        v = v * 10 + d;
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