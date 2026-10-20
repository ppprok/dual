#pragma once
#include "bit_chunk.h"
#include "bit_mask.h"

class BoolMatrixPreparation {
public:
    // Remove dependent rows (rows that are supersets of another row)
    static void RemoveDependentRows(bit_chunks& rows);

    //Remove dependent rows (supersets of another row) from the submatrix whose columns are given by the mask
    static void RemoveDependentRows(bit_chunks& rows, bit_mask const& mask);
};
