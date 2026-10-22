/// @file
/// @brief `BoolMatrixPreparation`: removal of dependent rows.

#pragma once
#include <dual/bit_chunk.h>
#include <dual/bit_mask.h>

/// @brief Preprocessing of Boolean matrices given as lists of row views.
class BoolMatrixPreparation {
public:
    /// @brief Remove dependent rows (rows that are supersets of another row); of equal rows one is kept.
    ///
    /// Removing them does not change the set of minimal covers. The order of the remaining rows is
    /// not preserved.
    static void RemoveDependentRows(bit_chunks& rows);

    /// @brief Remove dependent rows (supersets of another row) from the submatrix whose columns are
    ///        given by the mask; of rows equal on the mask one is kept.
    ///
    /// The rows themselves are not changed; the order of the remaining rows is not preserved.
    static void RemoveDependentRows(bit_chunks& rows, bit_mask const& mask);
};
