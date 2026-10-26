/// @file
/// @brief `CompletionBounds`: bounds on the weight that a search tree node can still gain, from
///        the irreducibility of the covers in its subtree.

#pragma once

#include <vector>

#include <dual/bit_matrix.h>
#include <dual/bits.h>
#include <dual/dualization_backtrack_algorithm_base.h>

namespace dual {

/// @brief Bounds on `f(C) - f(S)` over the minimal covers `C` that contain a search tree node `S`.
///
/// Every column `j` added to `S` on the way to a minimal cover `C` needs a private row: a row of the
/// matrix that `C` hits only in `j`. That row is not hit by `S`, and it can be taken among the
/// inclusion-minimal rows of the matrix (a row inside a private row is private too), so `C \ S` has
/// at most `u` columns, where `u` is the number of minimal rows not hit by `S`, each taken from
/// such a row, with different rows for different columns. Each of those rows also has to be hit by
/// some column of `C \ S`, and pairwise disjoint rows by different columns.
///
/// The functional is described by two gains per column, `positive[j] >= 0` and
/// `negative[j] <= 0`, such that for every minimal cover `C` containing `S`
/// `sum(negative[j]) <= f(C) - f(S) <= sum(positive[j])` over `j` in `C \ S`. For an additive
/// functional (`additive = true`, e.g. column weights) `f(C) - f(S)` must be exactly
/// `sum(positive[j] + negative[j])`, which tightens the bounds. `MaxGain` and `MinGain` then give
/// upper and lower bounds on `f(C) - f(S)` that hold for every algorithm whose subtree of `S`
/// reports only minimal covers containing `S` (all the search-tree algorithms of the library);
/// docs/optimal-covers.md has the proof.
///
/// Usage: `SetMatrix`, `SetGains`, then call `Update` for every new node before asking for the
/// bounds of that node. Backtrack nodes need no call: the state of a node is derived from its
/// parent's, which is kept for every depth of the current path.
class CompletionBounds {
public:
    /// @brief Set the matrix whose minimal covers are enumerated; drops the gains.
    void SetMatrix(bit_matrix const& matrix);

    /// @brief Set the gains of the columns (missing columns gain 0); call after `SetMatrix`.
    /// @param positive Gains `>= 0` (negative values are taken as 0).
    /// @param negative Gains `<= 0` (positive values are taken as 0).
    /// @param additive Whether `f(C) - f(S)` is exactly the sum of both gains over `C \ S`.
    /// @param magnitude Upper bound on the sum of the absolute values of the terms added to a node
    ///        weight on any path (used for the floating-point slack, see `Slack`).
    /// @param terms Upper bound on the number of terms added to a node weight on any path.
    void SetGains(Weights const& positive, Weights const& negative, bool additive, Weight magnitude, int terms);

    /// @brief Disable the bounds (`Enabled()` becomes `false`).
    void Clear();

    /// @brief Whether `SetMatrix` and `SetGains` were called.
    bool Enabled() const;

    /// @brief Whether `SetMatrix` was called (the gains may be missing).
    bool HasMatrix() const;

    /// @brief Width of the matrix set by `SetMatrix`.
    int Columns() const;

    /// @brief Track the rows not hit by a new node; other node types are ignored.
    void Update(DualizationNode const& node);

    /// @brief Upper bound on `f(C) - f(S)` for the last new node `S` (0 if `S` is a cover).
    double MaxGain();

    /// @brief Lower bound on `f(C) - f(S)` for the last new node `S` (0 if `S` is a cover).
    double MinGain();

    /// @brief Whether `MaxGain() < threshold`; stops as soon as the answer is known.
    bool MaxGainBelow(double threshold);

    /// @brief Whether `MinGain() > threshold`; stops as soon as the answer is known.
    bool MinGainAbove(double threshold);

    /// @brief Allowance for the rounding of the `float` node weights near @p weight.
    ///
    /// A cover weight is a `float` sum of its terms in the order of its path; it can differ from
    /// the exact sum by about `terms * FLT_EPSILON / 2 * (|weight| + magnitude)`. Comparing a bound
    /// widened by twice that much keeps the pruning admissible for the weights as computed, so
    /// pruned and full enumerations return the same covers even for fractional weights.
    double Slack(Weight weight) const;

private:
    typedef std::vector<bits64> Words;

    // Mask of the uncovered minimal rows of the node at depth d
    bits64 const* uncovered(int d) const {
        return uncovered_.data() + (size_t) d * rowWords_;
    }

    int CountUncovered() const;  // number of uncovered minimal rows of the last new node

    // The bounds; unless exact, they may stop early with any value on the same side of threshold
    double maxGain(double threshold, bool exact);
    double minGain(double threshold, bool exact);

    int columns_ = 0;          // matrix width
    int rowWords_ = 0;         // words per mask of rows
    int columnWords_ = 0;      // words per mask of columns
    int positiveColumns_ = 0;  // columns with a positive gain
    bool hasMatrix_ = false;   // SetMatrix called
    bool hasGains_ = false;    // SetGains called after SetMatrix
    bool additive_ = false;
    double slackScale_ = 0;  // 2 * (terms + 1) * FLT_EPSILON
    double magnitude_ = 0;
    int depth_ = 0;  // depth of the last new node

    std::vector<Words> matrixRows_;  // the inclusion-minimal rows of the matrix (distinct), by size
    Words columnRows_;               // for each column, the mask of the rows containing it
    Words rows_;                     // the same rows, columns renumbered by rank_, one after another
    Words uncovered_;                // masks of uncovered rows, one per depth of the current path
    Words union_;                    // scratch: union of the uncovered rows (by rank)
    Words packed_;                   // scratch: union of the packed rows (by rank)

    std::vector<double> positive_, negative_;  // gains by column
    std::vector<int> rank_;                    // position of a column by decreasing positive gain
    std::vector<double> rankedPositive_;       // positive gains by rank
    std::vector<int> byNegative_;              // columns with a negative gain, increasing gain
    std::vector<double> topPositive_;          // sums of the k largest positive gains, k = 0..
    std::vector<double> topNegative_;          // sums of the k smallest negative gains, k = 0..
    std::vector<double> maxPositive_, minPositive_, maxNegative_, minNegative_;  // by row
};

}  // namespace dual
