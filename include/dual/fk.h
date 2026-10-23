/// @file
/// @brief Khachiyan's incremental dualization: the Fredman–Khachiyan duality test (FK-B) and the
///        generation of minimal covers one by one.

#pragma once

#include <memory>

#include <dual/bit_matrix.h>
#include <dual/dualization_backtrack_algorithm_base.h>

namespace dual {

/// @brief Incremental dualization based on the Fredman–Khachiyan duality test (see `docs/references.md`).
///
/// The rows of a matrix are the edges of a hypergraph `H` over its columns, and its minimal covers
/// are the minimal transversals `Tr(H)`. Given a family `G` of minimal transversals found so far,
/// the duality test either proves `G = Tr(H)` or returns a witness: a transversal of `H` that
/// contains no set of `G`. The witness is reduced to a new minimal transversal, which is added to
/// `G`; this repeats until the test says "dual" (Boros, Elbassioni, Gurvich, Khachiyan, 2006).
/// The test is algorithm B of Fredman and Khachiyan (1996), so the time to find each next cover is
/// quasi-polynomial in the size of `H` and of the covers found so far.
namespace FK {

/// @brief Outcome of the duality test of two families of sets `H` and `G`.
enum class Duality {
    /// `G` is exactly the family of minimal transversals of `H` (and vice versa).
    Dual,
    /// Not dual; the witness is a transversal of `H` that contains no set of `G` (its complement
    /// contains no set of `H`), so `Tr(H)` has a set that is not in `G`.
    NewTransversal,
    /// Not dual; the witness contains a set of `G` and its complement contains a set of `H`, so a
    /// set of `G` misses a set of `H` (it is not a transversal).
    NotTransversal
};

/// @brief Result of `isDual`: the outcome and, if the families are not dual, a witness.
struct DualityResult {
    /// Outcome of the test.
    Duality result = Duality::Dual;
    /// Witness set of columns in increasing order (see `Duality`); empty if `result` is `Dual`.
    Columns witness;

    /// @brief Check whether the families are dual.
    bool dual() const {
        return result == Duality::Dual;
    }
};

/// @brief Fredman–Khachiyan duality test (algorithm B) of the families @p h and @p g.
///
/// Each family is given by the rows of a matrix (a row is the set of columns holding ones). Both
/// families are first reduced to their minimal sets (duplicates and supersets of other rows are
/// dropped), so the test is in fact about the monotone Boolean functions with these DNFs: `H` and
/// `G` are dual if and only if the minimal sets of `G` are exactly the minimal transversals of `H`.
/// So an empty `H` is dual only to `G = {{}}`, `H` with an empty row only to an empty `G`, and an
/// empty `G` only to `H` with an empty row. The columns are `0 .. max(h.width(), g.width()) - 1`.
/// @param h First family (the matrix whose covers are the transversals).
/// @param g Second family (candidate minimal covers of @p h).
/// @return `Duality::Dual`, or the kind of non-duality and a witness set of columns.
DualityResult isDual(bit_matrix const& h, bit_matrix const& g);

class Solver;

/// @brief Khachiyan's incremental generation of all minimal covers of the input matrix.
///
/// The rows of the input matrix (`setMatrix`, `loadMatrix`) are the sets to cover. Covers are
/// reported to `CoverCallback` as `DualizationNode::columns`, in increasing column order, each
/// once; `InnerCallback` is never called and `DualizationNode::weight` is not maintained, since
/// there is no backtracking search tree. Column restrictions (`setRestrictions`) are not supported
/// and are ignored. As in RUNC, a matrix without rows has one cover, the empty one, and a matrix
/// with a zero row has none.
///
/// - `dualize()` enumerates all covers from scratch in one run of the duality test: when a witness
///   is found deep in the recursion, the new cover is added to every family on the recursion path
///   and the run continues from there, so finished parts of the recursion are never repeated.
/// - `next()` finds one cover more than the covers found so far (`getCovers()`) or proves that
///   there is none; each call runs the duality test from the root. Calls of `next()` may follow
///   each other; `dualize()` and a change of the matrix restart from no covers.
/// - If `CoverCallback` stops the node (`DualizationNode::stoped`), the enumeration is finished
///   (`isDualizeFinished()`), as in RUNC.
///
/// Example:
/// @code
/// FK::GlobalState fk;
/// fk.loadMatrix("data/15_20_275.hg");
/// while (fk.next())
///     use(fk.getLastCover());  // 275 covers in total
/// @endcode
class GlobalState : public DualizationBacktrackAlgorithmBase {
public:
    /// @brief Algorithm with an empty input matrix.
    GlobalState();
    /// @brief Destructor.
    ~GlobalState() override;
    GlobalState(GlobalState const&) = delete;
    GlobalState& operator=(GlobalState const&) = delete;

    /// @brief Enumerate all minimal covers from scratch, reporting them to `CoverCallback`; stops
    ///        early if a callback sets `DualizationNode::stoped`.
    void dualize() override;

    /// @brief Find a minimal cover that is not among the covers found so far and report it to
    ///        `CoverCallback`.
    /// @return `true` if a new cover was found (it is `getLastCover()` and the last row of
    ///         `getCovers()`); `false` if all covers have been found or the enumeration was finished.
    bool next();

    /// @brief Forget the covers found so far; the next `next()` starts from the first cover.
    void resetDualize();

    /// @brief True when all covers have been found (or a callback stopped the enumeration).
    bool isDualizeFinished() const;

    /// @brief The covers found so far, one per row, in the order they were found.
    bit_matrix const& getCovers() const;

    /// @brief The last cover found, columns in increasing order (empty before the first one).
    Columns const& getLastCover() const;

protected:
    /// @brief Resets the enumeration (see `resetDualize`).
    void onMatrixChanged() override;

private:
    enum class Enumeration {
        NotStarted,
        Running,
        Finished
    };

    void start();
    bool run(bool once);
    bool reportCover(bit_chunk cover);

    Enumeration enumeration_ = Enumeration::NotStarted;
    bit_matrix hypergraph_;  // minimal rows of the input matrix
    bit_matrix covers_;      // covers found so far
    Columns lastCover_;
    std::unique_ptr<Solver> solver_;
};

}  // namespace FK

}  // namespace dual
