/// @file
/// @brief RUNC, RUNCM and OPT dualization algorithms with resumable, portion-wise enumeration.

#pragma once

#include <vector>

#include <dual/aot1.h>
#include <dual/bit_chunk.h>
#include <dual/bit_mask.h>
#include <dual/bit_vector.h>
#include <dual/dualization_backtrack_algorithm_base.h>
#include <dual/marked.h>
#include <dual/range.h>

namespace dual {

/// @brief RUNC family of dualization algorithms (Djukova, Prokofjev, see `docs/references.md`).
namespace Runc {

/// Rows of a matrix or submatrix, as views into the input `bit_matrix`.
typedef std::vector<bit_chunk> rows;
/// Index ranges of support-row groups, each marked with the column the rows support.
typedef std::vector<marked<range>> marked_ranges;
/// Iterator over `rows`.
typedef rows::iterator rows_iterator;
/// Constant iterator over `rows`.
typedef rows::const_iterator rows_citerator;

/// @brief Variant of the algorithm: how the columns to branch on are chosen in each node.
///
/// All variants enumerate the same set of minimal covers; they differ in the search tree.
enum AlgorithmType {
    /// Branch on all remaining columns after removing dependent rows (rows that are supersets of
    /// another uncovered row) and zero columns.
    OPT,
    /// Branch on the columns of the first uncovered row.
    RUNC,
    /// Branch on the columns of an uncovered row with the fewest remaining columns (the default).
    RUNCM
};

/// @brief Local state of a RUNC-type algorithm: one node of the search tree.
///
/// Holds the rows not yet covered by the current column set, the support rows of the chosen
/// columns (rows covered by exactly one chosen column, grouped by that column; a chosen column
/// whose group becomes empty is redundant, so the set cannot be extended to a minimal cover), and
/// the columns that may still be added. Row views point into the matrix of the owning
/// `RuncGlobalState`. Used internally by `RuncGlobalState`.
class RuncLocalState {
    enum AddResult {
        Inner,
        Cover,
        Uncompatable,
        Uncoverable
    };

    friend class RuncGlobalState;

private:
    // Uncovered rows
    rows uncovered_;

    // Support rows
    rows support_;

    // Groups of support rows
    marked_ranges supportGroups_;

    // Remaining columns
    bit_vector restCols_;
    bit_chunk collectedCols_;
    bit_vector temp_row_;

public:
    /// @brief State with room for @p m rows and @p n columns (nothing is allocated if either is 0).
    RuncLocalState(size_t m = 0, size_t n = 0);

    /// @brief Make this state the child of @p l obtained by adding column @p j.
    /// @param l Parent state.
    /// @param j Column to add; must be one of the parent's remaining columns.
    /// @return `Cover` if no uncovered rows remain, `Inner` if the branch continues,
    ///         `Uncoverable` if some uncovered row has no remaining column, `Uncompatable` if a
    ///         previously chosen column loses all its support rows (the set is not irredundant).
    Runc::RuncLocalState::AddResult AddColumn(RuncLocalState const& l, int j);

    /// @brief Remove column @p j from the remaining columns.
    void RemoveColumn(int j);

    /// @brief Set the rows of `[begin, end)` not covered by column @p col as the uncovered rows
    ///        (restricted to the remaining columns); the rows covered by @p col become its support rows.
    /// @return `false` if an uncovered row has no remaining column (the branch is dead).
    bool SetUncovered(int col, rows_citerator begin, rows_citerator end);

    /// @brief `SetUncovered(col, ...)` over the uncovered rows of @p parent.
    bool SetUncovered(RuncLocalState const& parent, int col);

    /// @brief Copy the support-row groups that stay support rows after adding column @p col
    ///        (rows that do not contain @p col).
    /// @return `false` if some group becomes empty (its column becomes redundant).
    bool SetUncoveredSupportRows(int col, marked_ranges const& groups, rows const& support);

    /// @brief `SetUncoveredSupportRows(col, ...)` over the support rows of @p parent.
    bool SetUncoveredSupportRows(RuncLocalState const& parent, int col);

    /// @brief Add the rows of `[begin, end)` that do not contain column @p col (all rows if
    ///        `col < 0`) as the support-row group of column @p l.
    ///
    /// Columns that belong to every row of the group are removed from the remaining columns,
    /// since adding one of them would make @p l redundant. If a row of the group has no remaining
    /// column, @p l can never lose its support and the group is dropped.
    /// @return `false` if the group is empty (column @p l would be redundant).
    bool AddUncoveredSupportRows(int col, int l, rows_citerator begin, rows_citerator end);

    /// @brief Check whether there are no uncovered rows, i.e. the chosen columns form a cover.
    bool IsCover() const;

    /// @brief Get the set of remaining columns (a view, not a copy).
    bit_mask getRestCols() const;

    /// @brief Make the remaining columns the union of the uncovered rows (the non-zero columns)
    ///        and return them.
    bit_chunk CollectRestCols();

    /// @brief Get the first uncovered row (there must be one).
    bit_chunk GetFrontUncovered() const;

    /// @brief Find the uncovered row with the fewest remaining columns.
    /// @param mc Return the first row with at most @p mc remaining columns immediately.
    bit_chunk GetMinUncovered(int mc = 0) const;

    /// @brief Remove dependent uncovered rows (rows that, restricted to the remaining columns, are
    ///        supersets of another row).
    void RemoveDependentRows();

    /// @brief Remove dependent rows and zero columns and get the remaining columns.
    bit_chunk GetUndepCols();

    /// @brief Columns to branch on in this node for algorithm variant @p alg (see `AlgorithmType`).
    bit_chunk collectCols(AlgorithmType alg);

    /// @brief Columns chosen for branching by `collectCols` and stored with `setCollectedCols`.
    const bit_chunk& getCollectedCols() const;
    /// @brief Store the columns to branch on.
    void setCollectedCols(bit_chunk collectedCols);
};

/// @brief Global state of a RUNC-type algorithm: enumerates the minimal covers of the input matrix.
///
/// The search is iterative, not recursive: a stack of `RuncLocalState` is advanced one step at a
/// time, so the enumeration can be paused and resumed. A minimal cover is a set of columns that
/// has a one in every row and has no proper subset with this property; each cover is reported once,
/// as `DualizationNode::columns` in the order the columns were added (not sorted).
///
/// Resumable enumeration contract:
/// - Input semantics: a matrix without rows has exactly one minimal cover, the empty one; a matrix
///   with an all-zero row has no covers. Neither case is an error.
/// - `dualize()` runs a complete enumeration from scratch and reports every cover to `CoverCallback`.
/// - `dualize(max_count)` and `dualize(buffer_size, buffer)` continue the current enumeration
///   (starting it if needed) and return the next portion of covers. They do not call
///   `CoverCallback`; the caller's `CoverCallback` is restored on return, also when an exception
///   propagates.
/// - `dualize(max_count)` returns an empty vector for `max_count > 0` only when the enumeration is
///   finished. `dualize(buffer_size, buffer)` writes `[size, col...]` records and returns their
///   count; 0 means finished only if `isDualizeFinished()`, otherwise the buffer was smaller than
///   `width() + 1` (or null) and nothing was done. After the end every further call returns nothing.
/// - `setMatrix()`, `loadMatrix()` and `mutable_matrix()` reset the enumeration: the next
///   portion starts from the first cover of the (new) matrix. So does `resetDualize()`.
/// - If a callback throws, the exception propagates and the enumeration is reset.
/// - If a callback stops the node (`DualizationNode::stoped`, e.g. `CoverCollector::MaxCount`), the
///   enumeration is finished: `isDualizeFinished()` is true, further portions return nothing, and
///   `resetDualize()` (or `dualize()`) starts it again from the first cover.
/// - The object is neither copyable nor movable; one object must not be used from several threads
///   at once.
///
/// Example:
/// @code
/// Runc::RuncGlobalState runc;              // RUNCM
/// runc.loadMatrix("data/15_20_275.hg");
/// for (auto covers = runc.dualize(100); !covers.empty(); covers = runc.dualize(100))
///     for (Columns const& cover : covers)
///         use(cover);                      // 275 covers in total
/// @endcode
class RuncGlobalState : public DualizationBacktrackAlgorithmBase {
private:
    enum class Enumeration {
        NotStarted,
        EmptyCover,  // the matrix has no rows: the empty cover is still to be reported
        Running,
        Finished
    };

    std::vector<RuncLocalState> _states;
    AlgorithmType _type;
    size_t state_depth_ = 0;
    Enumeration enumeration_ = Enumeration::NotStarted;

public:
    /// @brief Algorithm of variant @p type with an empty input matrix.
    RuncGlobalState(AlgorithmType type = AlgorithmType::RUNCM);

    /// @brief (Re)starts the enumeration from the first cover.
    void startDualize();

    /// @brief Drops the enumeration state; the next portion starts from the first cover.
    void resetDualize();

    /// @brief True when the started enumeration has reported all covers (or was stopped by a callback).
    bool isDualizeFinished() const;

    /// @brief Run a complete enumeration from scratch, reporting every cover to `CoverCallback` and
    ///        every inner node to `InnerCallback`.
    /// @throws Anything a callback throws; the enumeration is reset then.
    void dualize() override;

    /// @brief Continue the enumeration and return the next portion of covers.
    /// @param max_count Maximum number of covers to return.
    /// @return Up to @p max_count covers; empty (for `max_count > 0`) only when the enumeration is
    ///         finished.
    /// @throws Anything `InnerCallback` throws, or `std::bad_alloc`; the enumeration is reset then.
    std::vector<Columns> dualize(size_t max_count);

    /// @brief Continue the enumeration and write the next portion of covers into a buffer.
    ///
    /// Each cover is written as a record `[k, c_1, ..., c_k]`: its size, then its column indices.
    /// Covers are written while at least `width() + 1` words (the size of the largest possible
    /// record) are left, so nothing is written past @p buffer_size.
    /// @param buffer_size Number of words in @p buffer.
    /// @param buffer Output buffer.
    /// @return Number of records written. 0 means the enumeration is finished if
    ///         `isDualizeFinished()`; otherwise the buffer was null or smaller than `width() + 1`
    ///         and nothing was done.
    /// @throws Anything `InnerCallback` throws, or `std::bad_alloc`; the enumeration is reset then.
    size_t dualize(uint32_t buffer_size, uint32_t* buffer);

protected:
    /// @brief Resets the enumeration (see `resetDualize`).
    void onMatrixChanged() override;

private:
    void doDualize();
    bool doStep();

    RuncLocalState& getLocalState(std::size_t state_num);
};

}  // namespace Runc

}  // namespace dual
