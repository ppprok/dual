#pragma once
#include <functional>
#include <vector>

#include "bits.h"

/*! 
    Dualization algorithm RUNC-M for fixed width of input matrix: 32 or 64
    \param Row --- bits32 or bits64
*/
template<typename Row>
class RuncFixed {
public:
    typedef std::vector<Row> Rows;

private:
    struct SupportRow {
        int column;
        Row row;

        SupportRow(){};

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
    Row innerCount;

    Row resultsCount;

    Row extraCount;

    Rows idealIndep;

    Rows filterIndep;

    //! Output callback
    std::function<bool(RuncFixed*)> callback;

    //! Delay before make callback
    int callbackDelay;

    //! Default constructor of dualization algorithm
    RuncFixed();

    //! Dualize boolean matrix represented by rows
    bool Dualize(Rows A);

    //! Dualize boolean matrix represented by rows
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

template<typename T>
T RuncFixedDualize(std::vector<T> input, std::vector<T> input1) {
    RuncFixed<T> runc;
    runc.DualizeIntervals(std::move(input), std::move(input1));
    return runc.resultsCount;
}

template<typename T>
typename RuncFixed<T>::Rows RuncFixedDualize(typename RuncFixed<T>::Rows input) {
    RuncFixed<T> runc;
    runc.Dualize(std::move(input));
    return std::move(runc.idealIndep);
}
