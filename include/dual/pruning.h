/// @file
/// @brief `CoveredRowsWeighter`: pruning by the total weight of covered rows.

#pragma once

#include <dual/dualization_backtrack_algorithm_base.h>
#include <dual/weight_bounds_pruning.h>

/// @brief Pruning of search tree branches by bounds on the total weight of covered rows.
///
/// The weight of a node is the total weight of the target rows covered by its columns. Install it
/// as both `InnerCallback` (to weigh and prune inner nodes) and `CoverCallback` (to prune covers and
/// move dynamic bounds).
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
    /// @brief Rows and their weights.
    struct Options {
        /// Rows to be covered.
        bit_chunks TargetRows;

        /// Row weights, by row.
        Weights RowsWeights;

        /// Weight of the rows past the end of `RowsWeights`.
        Weight DefaultWeight;

        /// @brief No rows, default weight 0.
        Options() : DefaultWeight(0) {}
    };

public:
    /// @brief Weighter without rows and without bounds.
    CoveredRowsWeighter();

    /// @brief Reset the parameters to defaults: no rows, no bounds.
    void Reset();

    /// @brief Reset, then set the rows to cover and their weights.
    void SetOptions(Options options);

    /// @brief Recompute the weights and cut the branch if necessary.
    virtual void Call(DualizationNode& node);

    /// @brief Set a fixed upper bound; nodes heavier than it are pruned.
    void FixMaxBound(Weight bound);

    /// @brief Set a fixed lower bound; nodes lighter than it are pruned.
    void FixMinBound(Weight bound);

    /// @brief Search for the minimum value: each cover lowers the upper bound to its weight + epsilon.
    void TargetToMin();

    /// @brief Search for the maximum value: each cover raises the lower bound to its weight - epsilon.
    void TargetToMax();

    /// @brief Set the allowed deviation outside the interval (used by dynamic bounds).
    void SetEpsilon(Weight eps);

private:
    // Compute the weight of a new search tree node
    void WeightNewNode(DualizationNode& node);

    // Recompute the weight when backtracking one level up
    void WeightBacktrackNode(DualizationNode& node);
};
