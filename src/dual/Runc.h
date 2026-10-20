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

// Global state of a RUNC-type algorithm
class RuncGlobalState : public DualizationBacktrackAlgorithmBase {
private:

    std::vector<RuncLocalState> _states;
    AlgorithmType _type;
    size_t state_depth_ = 0;
    RuncLocalState* current_local_state_ = nullptr;

public:
    RuncGlobalState(AlgorithmType type = AlgorithmType::RUNCM);

    void startDualize();

    void dualize();
    std::vector<Columns> dualize(size_t max_count);
    size_t dualize(uint32_t buffer_size, uint32_t* buffer);

private:
    void doDualize();
    bool doStep();

    RuncLocalState& getLocalState(std::size_t state_num);
};

}  // namespace Runc