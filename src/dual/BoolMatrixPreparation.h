#pragma once
#include "bit_chunk.h"
#include "bit_mask.h"

class BoolMatrixPreparation {
public:
    // Удалить охватывающие строки
    static void RemoveDependentRows(bit_chunks& rows);

    //Удалить охватывающие строки из подматрицы, столбцы которой заданы маской
    static void RemoveDependentRows(bit_chunks& rows, bit_mask const& mask);
};
