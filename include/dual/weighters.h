/// @file
/// @brief Weighters: callbacks that maintain node weights and prune by weight bounds.

#pragma once

#include <vector>

#include <dual/bit_matrix.h>
#include <dual/dualization_backtrack_algorithm_base.h>
#include <dual/weight_bounds_pruning.h>

namespace dual {

/// @brief Base class for weighing search tree nodes and pruning them by weight bounds.
///
/// Install a weighter as `InnerCallback`: it maintains `DualizationNode::weight` for new and
/// backtrack nodes and prunes new nodes whose weight cannot get into `[min, max]` with the remaining
/// weights. Weights are indexed by column (`ColumnsWeighterCallback`) or by row
/// (`CoveredRowWeighterCallback`); the last weight is also the default for larger indices.
class WeighterBase : virtual public IDualizationCallback {
protected:
    /// Sum of the remaining positive weights.
    Weight _restPositive;

    /// Sum of the remaining negative weights.
    Weight _restNegative;

    /// Minimum allowed weight.
    Weight _minWeight;

    /// Maximum allowed weight.
    Weight _maxWeight;

    /// Weights.
    Weights _weights;

    /// Maximum number of terms in the sum (0: only the explicit weights, the default weight
    /// makes the sums unbounded).
    int _weightSumCount;

    /// States of the nodes on the current path, restored exactly on backtrack.
    WeightCheckpoints _checkpoints;

    /// @brief Invalidate the estimates of the sums of the remaining positive and negative terms.
    void InvalidateRestWeights();

    /// @brief Check whether the estimates of the sums of the remaining positive and negative terms are valid.
    bool IsRestWeightsValid() const;

    /// @brief Compute the estimates of the sums of the remaining positive and negative terms.
    void CalculateRestWeights();

    /// @brief Increase the total weight margin (return @p w to the remaining sums).
    void IncRestWeight(Weight w);

    /// @brief Decrease the total weight margin (remove @p w from the remaining sums).
    void DecRestWeight(Weight w);

public:
    /// @brief Weighter without weights and without bounds.
    WeighterBase();

    /// @brief Update the weight of a new or backtrack node; prune a new node if it cannot reach
    ///        the allowed interval.
    void WeightInnerNode(DualizationNode& node);

    /// @brief Set the weights; the last one is also the default weight.
    void SetWeights(Weights weights);

    /// @brief Weight of index @p i: 0 if negative, the default weight past the end.
    Weight GetWeight(int i);

    /// @brief The default weight: the last weight, or 0 if there are none.
    Weight GetDefaultWeight();

    /// @brief Reset to defaults: no weights, unbounded.
    void Reset();

    /// @brief Set the minimum allowed weight.
    void SetMinWeight(Weight w);

    /// @brief Set the maximum allowed weight.
    void SetMaxWeight(Weight w);

    /// @brief Sum of the remaining positive weights.
    Weight GetRestPositive() const;

    /// @brief Sum of the remaining negative weights.
    Weight GetRestNegative() const;

    /// @brief Prune the search tree branch by the allowed values of the current weight.
    /// @return `true` if no completion with the remaining weights gets into `[minWeight, maxWeight]`.
    bool Prun(Weight current, Weight minWeight, Weight maxWeight) const;

    /// @brief Prune the search tree branch by the allowed values of the node weight.
    ///
    /// A cover is pruned if its weight is outside `[minWeight, maxWeight]`, an inner node as in the
    /// overload for a weight. Sets `node.pruned`.
    /// @return `true` if the node is pruned (also if it already was).
    bool Prun(DualizationNode& node, Weight minWeight, Weight maxWeight) const;

    /// @brief Prune the search tree branch by the current allowed values of the node weight.
    /// @return `true` if the node is pruned.
    bool Prun(DualizationNode& node);

    /// @brief Add the weight brought by the last column of a new node to `node.weight`.
    virtual void WeightNewNode(DualizationNode& node) = 0;

    /// @brief Subtract the weight brought by the removed column of a backtrack node.
    virtual void WeightBacktrackNode(DualizationNode& node) = 0;

    /// @brief Same as `WeightInnerNode`.
    void Call(DualizationNode& node) override;
};

/// @brief Weighs search tree nodes by the total weight of columns.
class ColumnsWeighterCallback : public WeighterBase {

    // Compute the weight of a new search tree node
    void WeightNewNode(DualizationNode& node) override;

    // Recompute the weight when backtracking one level up
    void WeightBacktrackNode(DualizationNode& node) override;
};

/// @brief Weighs search tree nodes by the total weight of the target rows covered by their columns.
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
    /// @brief Weighter without rows, weights and bounds.
    CoveredRowWeighterCallback();

    /// @brief Reset the parameters to defaults.
    void Reset();  // NOLINT(bugprone-derived-method-shadowing-base-method): extends WeighterBase::Reset

    /// @brief Set the rows to be covered (all uncovered); row weights come from `SetWeights`.
    void SetTargetRows(bit_chunks targetRows);

    /// @brief Compute the weight of a new search tree node: add the weights of the target rows
    ///        first covered by its last column.
    void WeightNewNode(DualizationNode& node) override;

    /// @brief Recompute the weight when backtracking one level up.
    void WeightBacktrackNode(DualizationNode& node) override;
};

}  // namespace dual
