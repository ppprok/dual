/// @file
/// @brief `WeightBoundsPruningCallback`: pruning by the total weight of columns.

#pragma once

#include <vector>

#include <dual/completion_bounds.h>
#include <dual/dualization_backtrack_algorithm_base.h>

namespace dual {

/// @brief Weighter state saved on the way down the search tree and restored exactly on backtrack.
///
/// Weighters update `DualizationNode::weight` and their sums of remaining weights incrementally.
/// In floating point, adding a weight and subtracting it again does not always give back the old
/// value, so without checkpoints the weights drift over a long enumeration: the same cover gets
/// slightly different weights and tied optimal covers can be pruned by a dynamic bound. A weighter
/// saves its state before it adds the weight of a new node and restores it when that column is
/// removed, so the weight of a node depends only on its path from the root.
class WeightCheckpoints {
public:
    /// @brief Saved weights.
    struct State {
        Weight weight;        ///< Node weight.
        Weight restPositive;  ///< Sum of the remaining positive weights.
        Weight restNegative;  ///< Sum of the remaining negative weights.
    };

    /// @brief Save the state of the parent of the new node @p node (call before adding the weight
    ///        of `node.lastColumn`).
    void Save(DualizationNode const& node, State const& state);

    /// @brief Restore the state saved for the parent of the removed column (call for a backtrack
    ///        node, after the incremental update).
    /// @return `false` if no state was saved for this depth (then @p state is not changed).
    bool Restore(DualizationNode const& node, State& state);

    /// @brief Drop all saved states.
    void Clear();

private:
    std::vector<State> states_;
    std::vector<char> valid_;
};

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

    /// States of the nodes on the current path, restored exactly on backtrack.
    WeightCheckpoints _checkpoints;

    /// Bounds from irreducibility (see `EnableCompletionBounds`).
    CompletionBounds _completion;

    /// The column weights changed since the gains of `_completion` were computed.
    bool _completionDirty;

    /// @brief Recompute the gains of `_completion` from the column weights.
    void UpdateCompletionGains();

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
    void Call(DualizationNode& node) override;

    /// @brief Remove weight @p w from the sums of remaining weights.
    void DecRestWeight(Weight w);
    /// @brief Return weight @p w to the sums of remaining weights.
    void IncRestWeight(Weight w);
    /// @brief Prune an inner node that cannot reach the allowed interval with the remaining weights.
    void PrunInnerNode(DualizationNode& node);

    /// @brief Also prune by the irreducibility of the covers of @p matrix (`CompletionBounds`).
    ///
    /// Every column added to a node `S` of a minimal cover needs its own row not hit by `S`, so a
    /// node can gain at most the `u` largest positive weights of the columns of its `u` uncovered
    /// minimal rows (and at most the largest weight of each such row), and loses at least the
    /// cheapest column of every row of a disjoint set of uncovered rows. These bounds are much
    /// tighter than the sums of all remaining weights, above all for maximisation. @p matrix must
    /// be the matrix being dualized; the bounds follow later changes of the weights.
    void EnableCompletionBounds(bit_matrix const& matrix);

    /// @brief Stop pruning by `CompletionBounds` (also done by `Reset`).
    void DisableCompletionBounds();

    /// @brief Prune the new node @p node if @p bounds (updated for this node) show that no minimal
    ///        cover in its subtree gets into the allowed interval.
    void PrunByCompletion(DualizationNode& node, CompletionBounds& bounds) const;

    /// @brief Save `node.weight` and the remaining weights for the new node @p node, before its
    ///        last column is weighed (see `WeightCheckpoints`).
    void SaveCheckpoint(DualizationNode const& node);

    /// @brief Restore `node.weight` and the remaining weights saved for the backtrack node @p node.
    void RestoreCheckpoint(DualizationNode& node);
};

}  // namespace dual
