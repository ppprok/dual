#pragma once

#include <vector>

#include "AOT1.h"
#include "DualizationBacktrackAlgorithmBase.h"
#include "bit_chunk.h"
#include "bit_mask.h"
#include "bit_vector.h"
#include "marked.h"
#include "range.h"

namespace Runc {

typedef std::vector<bit_chunk> rows;
typedef std::vector<marked<range>> marked_ranges;
typedef rows::iterator rows_iterator;
typedef rows::const_iterator rows_citerator;

enum AlgorithmType {
    OPT,
    RUNC,
    RUNCM
};

// Local state of a RUNC-type algorithm
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

    RuncLocalState(size_t m = 0, size_t n = 0);

    Runc::RuncLocalState::AddResult AddColumn(RuncLocalState const& l, int j);

    // Remove a column
    void RemoveColumn(int j);

    // Set the rows not covered by column j
    bool SetUncovered(int col, rows_citerator begin, rows_citerator end);

    bool SetUncovered(RuncLocalState const& parent, int col);

    bool SetUncoveredSupportRows(int col, marked_ranges const& groups, rows const& support);

    bool SetUncoveredSupportRows(RuncLocalState const& parent, int col);

    bool AddUncoveredSupportRows(int col, int l, rows_citerator begin, rows_citerator end);

    bool IsCover() const;

    // Get the set of remaining columns
    bit_mask getRestCols() const;

    // Return the non-zero columns
    bit_chunk CollectRestCols();

    // Get the first uncovered row
    bit_chunk GetFrontUncovered() const;

    // Find the minimal uncovered row
    bit_chunk GetMinUncovered(int mc = 0) const;

    // Remove dependent rows (rows that are supersets of another row)
    void RemoveDependentRows();

    // Remove dependent rows and zero columns and get the remaining columns
    bit_chunk GetUndepCols();

    bit_chunk collectCols(AlgorithmType alg);

    const bit_chunk& getCollectedCols() const;
    void setCollectedCols(bit_chunk collectedCols);
};

// Global state of a RUNC-type algorithm.
//
// Resumable enumeration contract:
// - Input semantics: a matrix without rows has exactly one minimal cover, the empty one; a matrix
//   with an all-zero row has no covers. Neither case is an error.
// - dualize() runs a complete enumeration from scratch and reports every cover to CoverCallback.
// - dualize(max_count) and dualize(buffer_size, buffer) continue the current enumeration (starting
//   it if needed) and return the next portion of covers. They do not call CoverCallback; the
//   caller's CoverCallback is restored on return, also when an exception propagates.
// - dualize(max_count) returns an empty vector for max_count > 0 only when the enumeration is
//   finished. dualize(buffer_size, buffer) writes [size, col...] records and returns their count;
//   0 means finished only if isDualizeFinished(), otherwise the buffer was smaller than width()+1
//   (or null) and nothing was done. After the end every further call returns nothing.
// - setMatrix(), loadMatrix() and the non-const get_matrix() reset the enumeration: the next
//   portion starts from the first cover of the (new) matrix. So does resetDualize().
// - If a callback throws, the exception propagates and the enumeration is reset.
// - If a callback stops the node (DualizationNode::stoped, e.g. CoverCollector::MaxCount), the
//   enumeration is finished: isDualizeFinished() is true, further portions return nothing, and
//   resetDualize() (or dualize()) starts it again from the first cover.
// - The object is neither copyable nor movable; one object must not be used from several threads
//   at once.
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
    RuncGlobalState(AlgorithmType type = AlgorithmType::RUNCM);

    // (Re)starts the enumeration from the first cover
    void startDualize();

    // Drops the enumeration state; the next portion starts from the first cover
    void resetDualize();

    // True when the started enumeration has reported all covers
    bool isDualizeFinished() const;

    void dualize() override;
    std::vector<Columns> dualize(size_t max_count);
    size_t dualize(uint32_t buffer_size, uint32_t* buffer);

protected:
    void onMatrixChanged() override;

private:
    void doDualize();
    bool doStep();

    RuncLocalState& getLocalState(std::size_t state_num);
};

}  // namespace Runc