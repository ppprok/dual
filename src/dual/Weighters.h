#pragma once

#include <vector>

#include "DualizationBacktrackAlgorithmBase.h"
#include "bit_matrix.h"

//////////////////////////////////////////////////////////////////////////
// Base class for weighing search tree nodes
//////////////////////////////////////////////////////////////////////////
class WeighterBase : virtual public IDualizationCallback {
protected:
    // Sum of the remaining positive weights
    Weight _restPositive;

    // Sum of the remaining negative weights
    Weight _restNegative;

    // Minimum allowed weight
    Weight _minWeight;

    // Maximum allowed weight
    Weight _maxWeight;

    // Weights
    Weights _weights;

    // Maximum number of terms in the sum
    int _weightSumCount;

    // Invalidate the estimates of the sums of the remaining positive and negative terms
    void InvalidateRestWeights();

    // Check whether the estimates of the sums of the remaining positive and negative terms are valid
    bool IsRestWeightsValid() const;

    // Compute the estimates of the sums of the remaining positive and negative terms
    void CalculateRestWeights();

    // Increase the total weight margin
    void IncRestWeight(Weight w);

    // Decrease the total weight margin
    void DecRestWeight(Weight w);

public:
    WeighterBase();

    void WeightInnerNode(DualizationNode& node);

    void SetWeights(Weights weights);

    Weight GetWeight(int i);

    Weight GetDefaultWeight();

    void Reset();

    void SetMinWeight(Weight w);

    void SetMaxWeight(Weight w);

    Weight GetRestPositive() const;

    Weight GetRestNegative() const;

    // Prune the search tree branch by the allowed values of the current weight
    bool Prun(Weight current, Weight minWeight, Weight maxWeight) const;

    // Prune the search tree branch by the allowed values of the node weight
    bool Prun(DualizationNode& node, Weight minWeight, Weight maxWeight) const;

    // Prune the search tree branch by the current allowed values of the node weight
    bool Prun(DualizationNode& node);

    virtual void WeightNewNode(DualizationNode& node) = 0;

    virtual void WeightBacktrackNode(DualizationNode& node) = 0;

    void Call(DualizationNode& node);
};

// Weighs search tree nodes by the total weight of columns
class ColumnsWeighterCallback : public WeighterBase {

    // Compute the weight of a new search tree node
    void WeightNewNode(DualizationNode& node);

    // Recompute the weight when backtracking one level up
    void WeightBacktrackNode(DualizationNode& node);
};

// Weighs search tree nodes by the total weight of covered and uncovered rows
class CoveredRowWeighterCallback : public WeighterBase {
    // Rows that have to be covered
    bit_chunks _targetRows;

    typedef std::vector<int> IntVector;

    struct CoveredRow {
        int i;     // Row index
        int j;     // Column index
        Weight w;  // Row weight
    };

    // Independent rows
    std::vector<CoveredRow> _coveredRows;

    // Dependent rows
    IntVector _uncoveredRows;

public:
    CoveredRowWeighterCallback();

    // Reset the parameters to defaults
    void Reset();

    // Set the rows to be covered
    void SetTargetRows(bit_chunks targetRows);

    // Compute the weight of a new search tree node
    void WeightNewNode(DualizationNode& node);

    // Recompute the weight when backtracking one level up
    void WeightBacktrackNode(DualizationNode& node);
};