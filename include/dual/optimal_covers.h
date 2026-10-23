/// @file
/// @brief Optimal minimal (irreducible) covers: enumeration with branch pruning by weight bounds.

#pragma once

#include <dual/bit_matrix.h>
#include <dual/dualization_backtrack_algorithm_base.h>
#include <dual/weighted_cover.h>

namespace dual {

class WeighterBase;

/// @brief Direction of optimisation.
enum class Objective {
    Minimize,  ///< Look for the covers of the smallest weight.
    Maximize   ///< Look for the covers of the largest weight.
};

/// @brief Collects all covers of the optimal weight (ties included); install it as `CoverCallback`.
///
/// A cover is kept if its weight (`DualizationNode::weight`, maintained by a weighting callback) is
/// within `tolerance` of the best weight seen so far; when a strictly better cover arrives, the
/// covers that are no longer within the tolerance of it are dropped. Covers pruned by an earlier
/// callback (`DualizationNode::pruned`) are ignored. The collector does not prune the search by
/// itself: either chain it after a `WeightBoundsPruningCallback` (or `CoveredRowsWeighter`) with a
/// dynamic bound (`TargetToMin` / `TargetToMax` with epsilon = `tolerance`), or pass a
/// `WeighterBase` whose bound the collector moves to the best weight +- `tolerance`.
class OptimalCoverCollector : public IDualizationCallback {
public:
    /// @brief Collector for @p objective; covers within @p tolerance (>= 0) of the best weight count
    ///        as optimal.
    explicit OptimalCoverCollector(Objective objective = Objective::Minimize, Weight tolerance = 0);

    /// Weighter whose bound is moved to the best weight +- tolerance (not owned, may be null).
    WeighterBase* Weighter = nullptr;

    /// @brief Store the cover if it is (one of) the best so far.
    void Call(DualizationNode& node) override;

    /// @brief The optimal covers: columns as reported, in the order found.
    WeightedCovers const& GetCovers() const;

    /// @brief The best weight; meaningful only if `HasCovers()`.
    Weight GetBestWeight() const;

    /// @brief Whether at least one cover was collected.
    bool HasCovers() const;

    /// @brief Drop the collected covers and forget the best weight.
    void Reset();

private:
    bool better(Weight a, Weight b) const;
    bool withinTolerance(Weight w) const;

    Objective objective_;
    Weight tolerance_;
    Weight best_ = 0;
    bool hasBest_ = false;
    WeightedCovers covers_;
};

/// @brief Algorithm used by `optimalCovers`.
enum class CoverAlgorithm {
    RUNCM,  ///< `Runc::RuncGlobalState(Runc::RUNCM)` (default).
    RUNC,   ///< `Runc::RuncGlobalState(Runc::RUNC)`.
    OPT,    ///< `Runc::RuncGlobalState(Runc::OPT)`.
    AOT1,   ///< `AOT1::GlobalState` (variant AO1M).
    PUNC,   ///< `PUNC::GlobalState`, columns tried from the most promising weight.
    FK      ///< `FK::GlobalState`: no search tree, so no pruning, only filtering of all covers.
};

/// @brief Options of `optimalCovers` and `optimalCoversByCoveredRows`.
struct OptimalCoversOptions {
    /// Minimise or maximise the weight.
    Objective objective = Objective::Minimize;
    /// Covers within this distance of the optimum are also returned (>= 0). Keep 0 for weights
    /// whose sums are exact in `float` (e.g. integers up to 2^24); use a small positive value for
    /// fractional weights, whose sums depend on the order of the terms.
    Weight tolerance = 0;
    /// Enumeration algorithm.
    CoverAlgorithm algorithm = CoverAlgorithm::RUNCM;
    /// Prune branches by the dynamic weight bound; `false` enumerates all covers and filters them
    /// (for comparison). Ignored by `CoverAlgorithm::FK`, which never prunes.
    bool prune = true;
};

/// @brief Result of `optimalCovers`.
struct OptimalCovers {
    /// All optimal minimal covers (within the tolerance), columns in increasing order, covers in
    /// lexicographic order. Empty if the matrix has no cover (a zero row).
    WeightedCovers covers;
    /// The optimal weight (0 if there are no covers).
    Weight weight = 0;
    /// Number of new search tree nodes visited (`NodeType::NewNode`, pruned ones included).
    long long nodes = 0;
    /// Number of minimal covers reported by the algorithm (before pruning by the cover weight).
    long long coversVisited = 0;
};

/// @brief All minimal covers of @p matrix with the optimal total column weight.
///
/// The weight of a cover is the sum of @p columnWeights over its columns; columns past the end of
/// @p columnWeights weigh 1, so empty weights give the cover length. Weights may have any sign:
/// a branch is pruned only when even the remaining negative (for `Minimize`) or positive (for
/// `Maximize`) weights cannot bring it to the best weight found so far, so no optimal cover is lost.
OptimalCovers optimalCovers(bit_matrix const& matrix,
                            Weights const& columnWeights,
                            OptimalCoversOptions const& options = OptimalCoversOptions());

/// @brief All minimal covers of @p matrix with the optimal total weight of the covered target rows.
///
/// A target row (a row of @p targetRows, of the same width as @p matrix) is covered by a cover if
/// they share a column; its weight is in @p rowWeights (rows past the end weigh 1, weights must be
/// non-negative). Pruning bounds the weight from above by the weight of the target rows covered so
/// far plus the weight of the rest of them.
OptimalCovers optimalCoversByCoveredRows(bit_matrix const& matrix,
                                         bit_matrix const& targetRows,
                                         Weights const& rowWeights,
                                         OptimalCoversOptions const& options = OptimalCoversOptions());

}  // namespace dual
