#pragma once
#include "DualizationBacktrackAlgorithmBase.h"
#include "move_default.h"

// Покрытие с весом
struct WeightedCover {
    Columns columns;
    Weight weight;

    WeightedCover() {}

    WeightedCover(Columns c, Weight w = 0) {
        columns = std::move(c);
        weight = w;
    }

    UTILITY_MOVE_DEFAULT_MEMBERS(WeightedCover, (columns) (weight));

    Columns::const_iterator begin() const {
        return columns.begin();
    }

    Columns::const_iterator end() const {
        return columns.end();
    }
};

typedef std::vector<WeightedCover> WeightedCovers;
