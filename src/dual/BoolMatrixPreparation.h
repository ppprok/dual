#pragma once
#include "bit_chunk.h"
#include "bit_mask.h"

class BoolMatrixPreparation {
public:
    // ������� ������������ ������
    static void RemoveDependentRows(bit_chunks& rows);

    //������� ������������ ������ �� ����������, ������� ������� ������ ������
    static void RemoveDependentRows(bit_chunks& rows, bit_mask const& mask);
};
