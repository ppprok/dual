/// @file
/// @brief `RuncFixed`: RUNC-M for matrices with at most 32 or 64 columns.

#pragma once
#include <functional>
#include <vector>

#include <dual/bits.h>

namespace dual {

/// @brief Dualization algorithm RUNC-M for a fixed width of the input matrix: 32 or 64 columns.
///
/// Rows and results are bit masks of type @p Row (bit `j` is column `j`). Results are collected in
/// `idealIndep` (and `filterIndep`); `callback` is called every `callbackDelay` results and once at
/// the end.
/// @tparam Row `bits32` or `bits64` (explicitly instantiated in `RuncFixed.cpp`).
template<typename Row>
class RuncFixed {
public:
    /// Rows of a matrix, or result column sets, as bit masks.
    typedef std::vector<Row> Rows;

private:
    struct SupportRow {
        int column;
        Row row;

        SupportRow() {};

        SupportRow(int c, Row r) {
            column = c;
            row = r;
        }
    };

    typedef std::vector<SupportRow> SupportRows;

    struct LocalState {
        Rows uncoveredRows;
        SupportRows supportRows;
        Row compatable;
        Row current;
        Row candidates;
    };

    typedef std::vector<LocalState> LocalStates;

    LocalStates _idealStates;

    LocalStates _filterStates;

    int _callbackTick;

public:
    /// Number of inner nodes of the last run.
    Row innerCount;

    /// Number of results of the last run.
    Row resultsCount;

    /// Number of dead-end nodes of the last run (an uncovered row without allowed columns).
    Row extraCount;

    /// Results of `Dualize` (minimal covers of `A`), or the covers of `A` from `DualizeIntervals`.
    Rows idealIndep;

    /// Covers of `B` from `DualizeIntervals`, paired with `idealIndep` by index.
    Rows filterIndep;

    /// @brief Output callback: called every `callbackDelay` results and after the last one;
    ///        returning `false` stops the enumeration. `idealIndep`/`filterIndep` keep growing.
    std::function<bool(RuncFixed*)> callback;

    /// Number of results between two calls of `callback` (100 by default).
    int callbackDelay;

    /// @brief Default constructor of the dualization algorithm.
    RuncFixed();

    /// @brief Dualize the Boolean matrix represented by rows: enumerate its minimal covers.
    /// @param A Rows of the matrix.
    /// @return `false` if `callback` stopped the enumeration.
    bool Dualize(Rows A);

    /// @brief Jointly dualize two matrices: enumerate pairs (X, Y) of disjoint column sets, X a
    ///        minimal cover of @p A and Y a minimal cover of @p B.
    /// @return `false` if `callback` stopped the enumeration.
    bool DualizeIntervals(Rows A, Rows B);

private:
    LocalState& GetLocalState(LocalStates& localStates, int i);

    //! Recursive call for dualization
    bool DoDualize(int level);

    //! Recursive call for dualization
    bool DoDualize(int level, int level1);

    //! Output result from local state
    bool Output(LocalState const& state);

    bool Output(LocalState const& state, LocalState const& state1);

    //! Collect unsupported columns
    Row GetLastSupportMask(SupportRows& supportRows);

    //! Create child state
    bool CreateChild(LocalState const& parent, int column, Row const& columnMask, LocalState& child);

    //! Find uncovered row with minimum count of bits
    Row FindRow(Rows const& uncoveredRows, Row const& restColumns) const;

    void InitLocalStates(Rows rows, LocalStates& localStates);

    bool DoCallback();

    bool IsExtra(LocalState& state, Row mask) const;
};

/// @brief Run `RuncFixed<T>::DualizeIntervals(input, input1)`.
/// @tparam T `bits32` or `bits64` (explicitly instantiated in `RuncFixed.cpp`).
/// @return The number of results.
template<typename T>
T RuncFixedDualize(std::vector<T> input, std::vector<T> input1);

/// @brief Run `RuncFixed<T>::Dualize(input)`.
/// @tparam T `bits32` or `bits64` (explicitly instantiated in `RuncFixed.cpp`); not deduced, so
///         call it as `RuncFixedDualize<bits64>(rows)`.
/// @return The minimal covers of @p input as bit masks.
template<typename T>
typename RuncFixed<T>::Rows RuncFixedDualize(typename RuncFixed<T>::Rows input);

}  // namespace dual
