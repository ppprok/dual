/// @file
/// @brief AOT1: asymptotically optimal dualization algorithms AO1/AO2 and their variants.

#pragma once

#include <vector>

#include "DualizationBacktrackAlgorithmBase.h"
#include "bit_chunk.h"
#include "marked.h"

/// @brief Asymptotically optimal dualization algorithms of type AO1/AO2 (Djukova).
///
/// The search adds pairs (uncovered row, column of that row); rows of earlier pairs that the new
/// column also covers become "competing" rows that must be covered later. Every minimal cover is
/// reported once.
namespace AOT1 {

//typedef raw_vector<marked<range>> marked_ranges;
/// Rows of the matrix, as views into the input `bit_matrix`.
typedef std::vector<bit_chunk> rows;
/// Iterator over `rows`.
typedef rows::iterator rows_iterator;
/// Constant iterator over `rows`.
typedef rows::const_iterator rows_citerator;

/// @brief Variant of the algorithm: which columns are branched on in each node.
///
/// All variants enumerate the same set of minimal covers.
enum AlgorithmType {
    AO1,   ///< All remaining columns.
    AO2,   ///< All remaining columns, after removing dependent (superset) uncovered rows.
    AO1K,  ///< Columns of the first competing (or else uncovered) row.
    AO2K,  ///< As `AO1K`, after removing dependent uncovered rows.
    AO1M,  ///< Columns of the row with the fewest remaining columns (the default).
    AO2M   ///< As `AO1M`, after removing dependent uncovered rows.
};

class GlobalState;

/// @brief Local state of an AOT1-type algorithm: one node of the search tree.
///
/// Used internally by `GlobalState`; row views point into the input matrix and the remaining
/// columns into a buffer owned by the `GlobalState`.
class LocalState {

    friend class GlobalState;

    enum AddResult {
        Inner,
        Cover,
        Uncompatable,
        Uncoverable
    };

private:
    // Uncovered rows
    rows _uncovered;

    // Competing rows
    rows _competing;

    // Remaining columns
    bit_chunk _restCols;

public:
    /// @brief Empty state; call `Init` before use.
    LocalState(){};

    /// @brief Move the rows of @p s; the remaining-columns view is copied.
    LocalState(LocalState&& s);

    /// @brief Move the rows of @p s; the remaining-columns view is copied.
    LocalState& operator=(LocalState&& s);

    /// @brief Initialize the local state: reserve rows and bind the remaining columns to the
    ///        buffer row of this state in @p g.
    void Init(GlobalState& g);

    /// @brief Make this state the child of @p l obtained by adding the pair (uncovered row @p i,
    ///        column @p j).
    /// @return `Cover` if no uncovered or competing rows remain, `Inner` if the branch continues,
    ///         `Uncoverable` if only competing rows remain.
    AddResult AddItem(GlobalState& g, LocalState const& l, int i, int j);

    /// @brief Remove column @p j from the remaining columns.
    void RemCol(int j);

    /// @brief Check whether there are no uncovered and no competing rows.
    bool IsCover() const;

    /// @brief Get the set of remaining columns.
    bit_chunk GetRestCols() const;

    /// @brief Make the remaining columns the union of the uncovered rows (the non-zero columns)
    ///        and return them.
    bit_chunk CollectRestCols();

    /// @brief Get the first competing row, or the first uncovered row if there are none.
    bit_chunk GetFrontUncovered() const;

    /// @brief Find the competing or uncovered row with the fewest remaining columns.
    /// @param mc Return the first row with at most @p mc remaining columns immediately.
    bit_chunk GetMinUncovered(int mc = 0) const;

    /// @brief Remove covering rows: uncovered rows that, restricted to the remaining columns, are
    ///        supersets of another row.
    void RemoveDependentRows();

    /// @brief Restrict the remaining columns to the union of the uncovered rows and return them.
    bit_chunk GetNonZeroCols(GlobalState& g);

    /// @brief Columns to branch on in this node for variant @p alg (see `AlgorithmType`).
    bit_chunk GetCols(GlobalState& g, AlgorithmType alg);
};

/// @brief Global state of an AOT1-type algorithm: enumerates all minimal covers of the input
///        matrix, recursively.
///
/// Reports covers to `CoverCallback` (as `DualizationNode::columns`, in the order the columns were
/// added) and inner nodes to `InnerCallback`. A matrix without rows has one cover, the empty one;
/// a matrix with a zero row has none.
class GlobalState : public DualizationBacktrackAlgorithmBase {
private:
    friend class LocalState;

    typedef std::vector<LocalState> LocalStates;

    bit_matrix _tempMatrix;

    bit_chunk _tempRow;

    LocalStates _states;

    AlgorithmType _type;

public:
    /// @brief Algorithm of variant @p type with an empty input matrix.
    GlobalState(AlgorithmType type = AlgorithmType::AO1M) : _type(type) {}

    /// @brief Enumerate all minimal covers (see the class description); stops early if a callback
    ///        sets `DualizationNode::stoped`.
    void dualize() override;

private:
    bool DoDualize(int stateNum);

    LocalState* GetLocalState(int stateNum);
};

}  // namespace AOT1
