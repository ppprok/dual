/// @file
/// @brief `WeightBoundsPruningCallback`: pruning by the total weight of columns.

#pragma once

#include "DualizationBacktrackAlgorithmBase.h"

/// @brief Pruning of search tree branches by bounds on the total weight of columns.
///
/// The weight of a node is the total weight of its columns. A new node is pruned when even the
/// remaining negative (positive) weights cannot bring it inside `[min, max]`; a cover is pruned
/// when its weight is outside the bounds. Install it as both `InnerCallback` and `CoverCallback`.
class WeightBoundsPruningCallback : public IDualizationCallback {
protected:
    /// Column weights.
    Weights _weights;

    /// Minimum allowed weight.
    Weight _minWeight;

    /// Maximum allowed weight.
    Weight _maxWeight;

    /// Sum of the remaining positive weights.
    Weight _restPositive;

    /// Sum of the remaining negative weights.
    Weight _restNegative;

    /// Dynamically move the lower bound to (maximum weight - _epsilon).
    bool _dynamicMinBound;

    /// Dynamically move the upper bound to (minimum weight + _epsilon).
    bool _dynamicMaxBound;

    /// Offset of the dynamic bound from the minimum (maximum) weight found.
    Weight _epsilon;

    /// Weight of the columns that have no weight in the _weights array.
    Weight _defaultWeight;

public:
    /// @brief Process a cover: prune it if its weight is outside the bounds, then move the
    ///        dynamic bounds.
    void PrunCoverNode(DualizationNode& node);

    /// @brief Callback without weights and without bounds.
    WeightBoundsPruningCallback();

    /// @brief Reset the parameters to defaults: no weights, unbounded, epsilon and default weight 0.
    void Reset();

    /// @brief Set a fixed upper bound (disables the dynamic upper bound).
    void FixMaxBound(Weight bound);

    /// @brief Set a fixed lower bound (disables the dynamic lower bound).
    void FixMinBound(Weight bound);

    /// @brief Search for the minimum value: each cover lowers the upper bound to its weight + epsilon.
    void TargetToMin();

    /// @brief Search for the maximum value: each cover raises the lower bound to its weight - epsilon.
    void TargetToMax();

    /// @brief Set the allowed deviation outside the interval (used by dynamic bounds).
    void SetEpsilon(Weight eps);

    /// @brief Set the weight of the columns without an explicit weight.
    void SetDefaultWeight(Weight w);

    /// @brief Set the column weights and recompute the sums of remaining weights.
    void SetWeights(Weights weights);

    /// @brief Get the weight of column j (the default weight if it has none).
    Weight GetWeight(int j) const;

    /// @brief Check whether column @p j has no explicit weight.
    bool IsDefaultWeight(int j) const;

    /// @brief Recompute the weights and cut the branch if necessary.
    virtual void Call(DualizationNode& node);

    /// @brief Remove weight @p w from the sums of remaining weights.
    void DecRestWeight(Weight w);
    /// @brief Return weight @p w to the sums of remaining weights.
    void IncRestWeight(Weight w);
    /// @brief Prune an inner node that cannot reach the allowed interval with the remaining weights.
    void PrunInnerNode(DualizationNode& node);
};
