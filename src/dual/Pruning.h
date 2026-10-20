#pragma once

#include "DualizationBacktrackAlgorithmBase.h"
#include "WeightBoundsPruning.h"

// Pruning of search tree branches by bounds on the total weight of covered rows
class CoveredRowsWeighter : public IDualizationCallback {
    // Rows that have to be covered
    bit_chunks _targetRows;

    typedef std::vector<int> IntVector;

    struct CoveredRow {
        int i;     // Row index
        int j;     // Column index
        Weight w;  // Row weight
    };

    // Covered rows
    std::vector<CoveredRow> _coveredRows;

    // Dependent rows
    IntVector _uncoveredRows;

    // Computes the weights of covered rows
    WeightBoundsPruningCallback _pruner;

public:
    struct Options {
        // Rows to be covered
        bit_chunks TargetRows;

        // Row weights
        Weights RowsWeights;

        // Default weight
        Weight DefaultWeight;

        Options() : DefaultWeight(0) {}
    };

public:
    CoveredRowsWeighter();

    // Reset the parameters to defaults
    void Reset();

    // Set the default weight
    void SetOptions(Options options);

    // Recompute the weights and cut the branch if necessary
    virtual void Call(DualizationNode& node);

    // Set a strict upper bound
    void FixMaxBound(Weight bound);

    // Set a strict lower bound
    void FixMinBound(Weight bound);

    // Search for the minimum value
    void TargetToMin();

    // Search for the maximum value
    void TargetToMax();

    // Set the allowed deviation outside the interval
    void SetEpsilon(Weight eps);

private:
    // Compute the weight of a new search tree node
    void WeightNewNode(DualizationNode& node);

    // Recompute the weight when backtracking one level up
    void WeightBacktrackNode(DualizationNode& node);
};
