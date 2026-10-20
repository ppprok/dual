#pragma once

#include "DualizationBacktrackAlgorithmBase.h"

// Pruning of search tree branches by bounds on the total weight of columns
class WeightBoundsPruningCallback : public IDualizationCallback {
protected:
    // Column weights
    Weights _weights;

    // Minimum allowed weight
    Weight _minWeight;

    // Maximum allowed weight
    Weight _maxWeight;

    // Number of positive weights
    Weight _restPositive;

    // Number of negative weights
    Weight _restNegative;

    // Dynamically move the lower bound to (maximum weight - _epsilon)
    bool _dynamicMinBound;

    // Dynamically move the upper bound to (minimum weight + _epsilon)
    bool _dynamicMaxBound;

    // Offset of the dynamic bound from the minimum (maximum) weight found
    Weight _epsilon;

    // Weight of the columns that have no weight in the _weights array;
    Weight _defaultWeight;

public:
    // Process a cover
    void PrunCoverNode(DualizationNode& node);

    WeightBoundsPruningCallback();

    // Reset the parameters to defaults
    void Reset();

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

    // Set the default weight
    void SetDefaultWeight(Weight w);

    // Set the weights
    void SetWeights(Weights weights);

    // Get the weight of column j
    Weight GetWeight(int j) const;

    bool IsDefaultWeight(int j) const;

    // Recompute the weights and cut the branch if necessary
    virtual void Call(DualizationNode& node);

    void DecRestWeight(Weight w);
    void IncRestWeight(Weight w);
    void PrunInnerNode(DualizationNode& node);
};
