/// @file
/// @brief PUNC dualization algorithm with weighted row and column orders.

#pragma once

#include <random>
#include <vector>

#include "DualizationBacktrackAlgorithmBase.h"
#include "bit_mask.h"
#include "bit_vector.h"

/// @brief Assignment of a weight to an ordinal number (of a row or a column).
struct OrderByWeight {
    /// Row or column index.
    int order;
    /// Its weight; smaller weights come first.
    Weight weight;

    /// @brief Index @p o with weight @p w.
    OrderByWeight(int o = -1, Weight w = 0) : order(o), weight(w) {}
};

/// Indices with weights, e.g. a traversal order.
typedef std::vector<OrderByWeight> OrderByWeights;

/// @brief PUNC dualization algorithm: covers the rows one by one in a given order.
namespace PUNC {

/// Support rows as pairs (column, row): the row is covered only by that chosen column.
typedef std::vector<std::pair<int, int>> SupportRows;

/// @brief Local state of the PUNC algorithm: one node of the search tree.
struct LocalState {

    /// Position (in the row order) of the first row not covered by `usedColumns`.
    int topRow;

    /// Support rows of the chosen columns.
    SupportRows supportRows;

    /// Chosen columns.
    bit_vector usedColumns;

    /// Columns of the top row that may be added.
    bit_vector restColumns;

    /// `restColumns` in the order they are tried.
    OrderByWeights columnsOrder;

    /// @brief Empty state.
    LocalState() : topRow(-1) {};
};

/// Stack of local states.
typedef std::vector<LocalState> LocalStates;

/// @brief Global state of the PUNC algorithm: enumerates all minimal covers of the input matrix,
///        recursively.
///
/// Each node covers the first uncovered row (in the row order) with one of its columns; columns
/// are tried in increasing order of their weights, columns with equal weights in random order.
/// Reports covers to `CoverCallback` and inner nodes to `InnerCallback`. A matrix without rows has
/// one cover, the empty one; a matrix with a zero row has none.
class GlobalState : public DualizationBacktrackAlgorithmBase {

    LocalStates _states;

    bit_vector _tempRow;

    // Order of rows
    OrderByWeights _rowsOrder;

    // Order of columns (column weights)
    Weights _colsWeights;

    // Source of the random order of columns with equal weights
    std::mt19937 _random;

public:
    /// @brief Set the row traversal order: rows are covered in increasing order of @p weights
    ///        (missing weights are 0, ties in random order).
    /// @note Uses the height of the current matrix; call it after setting the matrix.
    void SetRowsOrder(Weights const& weights);

    /// @brief Set the column traversal order: columns are tried in increasing order of @p weights
    ///        (missing weights are 0, ties in random order).
    void SetColsOrder(Weights weights);

    /// @brief Perform full dualization (see the class description); stops early if a callback
    ///        sets `DualizationNode::stoped`.
    void dualize() override;

private:
    // Perform dualization starting from the given level
    bool DoDualize(int level);

    // Get the local state by level
    LocalState& GetLocalState(int level);

    // Find the top uncovered row
    int FindTopUncoveredRow(LocalState& state);

    // Get the row number by the order in which it must be covered
    int GetRowOrder(int i) const;

    // Initialize the root state
    void InitState(LocalState& state);

    // Get the set of uncovered columns
    bool CollectRestCols(LocalState& state);

    // Initialize the child state
    void InitChildState(LocalState& child, LocalState const& parent, int j);

    void SetOrder(Weights const& weights, int n, OrderByWeights& order);

    // Sort by increasing weights
    void Sort(OrderByWeights& order);

    // Form the set of columns in the required order
    void CollectColsOrder(bit_chunk bits, OrderByWeights& order);
};

}  // namespace PUNC
