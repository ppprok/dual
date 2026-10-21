/// @file
/// @brief `WeightedCover`: a cover with a weight.

#pragma once
#include "DualizationBacktrackAlgorithmBase.h"

/// @brief Cover with a weight.
struct WeightedCover {
    /// Columns of the cover.
    Columns columns;
    /// Weight of the cover.
    Weight weight;

    /// @brief Empty cover with an uninitialised weight.
    WeightedCover() {}

    /// @brief Cover with columns @p c and weight @p w.
    WeightedCover(Columns c, Weight w = 0) {
        columns = std::move(c);
        weight = w;
    }

    /// @brief Iterator to the first column.
    Columns::const_iterator begin() const {
        return columns.begin();
    }

    /// @brief Iterator past the last column.
    Columns::const_iterator end() const {
        return columns.end();
    }
};

/// List of weighted covers.
typedef std::vector<WeightedCover> WeightedCovers;
