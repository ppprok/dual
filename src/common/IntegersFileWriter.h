#pragma once

#include "bit_matrix.h"
#include "io_utils.h"

typedef std::vector<int> Positions;

// Реализация вывода целых чисел файл
class IntegersFileWriter {

    FILE_ptr _output;

public:
    IntegersFileWriter(std::string const& filename);

    IntegersFileWriter();

    // Закрыть файл
    void CloseOutputFile();

    // Создать файл для записи
    void CreateOutputFile(std::string const& outputFileName);

    // Символ
    void WriteChar(char c) {
        putc(c, _output.get());
    }

    // Цифра
    void WriteDigit(int digit) {
        WriteChar(digit + '0');
    }

    // Конец строки
    void WriteEOL() {
        WriteChar('\n');
    }

    // Вывести число в файл
    void WriteInteger(int i, int pad = 0);

    void WriteNumber(double v, int precision = 3, int pad = 0);

    template<typename C>
    void WriteIntegers(C const& c) {
        WriteIntegers(c.begin(), c.end());
    }

    // Вывести список чисел в файл
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

    // Записать вершины набор массивов в файл --- каждый массив в отдельной строке
    // [beg, end) --- список, каждый элемент которого является массивом целых
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

    // Записать позиции ненулевых битов
    void WriteBitNumbers(bit_chunk bits);

    // Записать булеву матрицу в виде гиперграфа
    void WriteBitMatrix(bit_matrix const& bm);

    // Записать булеву матрицу в виде гиперграфа
    void WriteBitMatrix(bit_chunks const& rows);
};
