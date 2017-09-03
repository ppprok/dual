#include "stdafx.h"
#include "RuncFixed.h"


template<typename Row>
bool RuncFixed<Row>::Output( LocalState const& result )
{
    ++resultsCount;
    idealIndep.push_back(result.current);
    ++_callbackTick;

    if (_callbackTick == callbackDelay)
        return DoCallback();

    return true;
}

template<typename Row>
bool RuncFixed<Row>::Output(LocalState const& maximal, LocalState const& minimal)
{
    ++resultsCount;
    idealIndep.push_back(maximal.current);
    filterIndep.push_back(minimal.current);
    ++_callbackTick;

    if (_callbackTick == callbackDelay)
        return DoCallback();

    return true;
}


template<typename Row>
bool RuncFixed<Row>::DualizeIntervals( Rows A, Rows B )
{
    InitLocalStates(std::move(A), _idealStates);
    InitLocalStates(std::move(B), _filterStates);

    innerCount = 0;
    resultsCount = 0;
    extraCount = 0;
    _callbackTick = 0;
    idealIndep.clear();
    filterIndep.clear();

    bool r = DoDualize(0, 0);
    
    if (r && _callbackTick != 0)
        r = DoCallback();

    return r;
}


template<typename Row>
bool RuncFixed<Row>::Dualize( Rows A )
{
    InitLocalStates(std::move(A), _idealStates);
    
    innerCount = 0;
    resultsCount = 0;
    extraCount = 0;
    _callbackTick = 0;
    idealIndep.clear();
    filterIndep.clear();

    bool r = DoDualize(0);

    if (r && _callbackTick != 0)
        r = DoCallback();

    return r;
}

template<typename Row>
typename RuncFixed<Row>::LocalState& RuncFixed<Row>::GetLocalState(LocalStates& localStates, int i )
{
    assert(i >=0 && i < (int)localStates.capacity());

    if(i >= (int)localStates.size())
    {
        localStates.resize(i+1);
        int m = localStates.front().uncoveredRows.size();
        if (m)
        {
            localStates[i].supportRows.reserve(m);
            localStates[i].uncoveredRows.reserve(m);
        }
    }
    return localStates[i];
}

template<typename Row>
bool RuncFixed<Row>::IsExtra(LocalState& state, Row mask) const
{
    if (state.uncoveredRows.empty())
    {
        state.candidates = 0;
        return false;
    }
    
    state.candidates = FindRow(state.uncoveredRows, mask);
    return state.candidates == 0;
}

template<typename Row>
bool RuncFixed<Row>::DoDualize( int level)
{
    auto& state = _idealStates[level];
    
    if (IsExtra(state, state.compatable))
    {
        ++extraCount;
        return true;
    }

    if (state.uncoveredRows.empty())
        return Output(state);

    ++innerCount;
        
    auto& child = GetLocalState(_idealStates, level+1);    
    for (low_bit<Row> lb(state.candidates); lb; ++lb)
    {
        int j = lb;
        Row mask = ((Row)1) <<j;
        state.compatable &= ~mask;
        CreateChild(state, j, mask, child);
        if (! DoDualize(level+1))
            return false;
    }

    return true;
}


template<typename Row>
bool RuncFixed<Row>::DoDualize( int level, int level1)
{
    auto& state = GetLocalState(_idealStates, level);                
    auto& state1 = GetLocalState(_filterStates, level1);

    if (IsExtra(state, state.compatable & (~state1.current)) || 
        IsExtra(state1, state1.compatable & (~state.current)))
    {
        ++extraCount;
        return true;
    }
        
    if (state.uncoveredRows.empty() && state1.uncoveredRows.empty())
        return Output(state, state1);
    
    ++innerCount;

    if (! state.candidates || (state1.candidates && (bits_count(state.candidates) > bits_count(state1.candidates))))
    {
        auto& child1 = GetLocalState(_filterStates, level1+1);
        auto comp = state.compatable;
        for (low_bit<Row> lb(state1.candidates); lb; ++lb)
        {
            int j = lb;            
            Row mask = ((Row)1) <<j;
            state1.compatable &= ~mask;            
            CreateChild(state1, j, mask, child1);
            if (! DoDualize(level, level1+1))
                return false;
            state.compatable = comp;
        }

    }
    else
    {
        auto& child = GetLocalState(_idealStates, level+1);
        auto comp = state1.compatable;
        for (low_bit<Row> lb(state.candidates); lb; ++lb)
        {
            int j = lb;
            Row mask = ((Row)1) <<j;
            state.compatable &= ~mask;
            CreateChild(state, j, mask, child);
            if (! DoDualize(level+1, level1))
                return false;
            state1.compatable = comp;
        }
    }
    
    return true;    
}

template<typename Row>
Row RuncFixed<Row>::GetLastSupportMask( SupportRows& supportRows )
{
    assert(! supportRows.empty());
    auto mask = supportRows.back().row;
    auto j = supportRows.back().column;
    for (int i = supportRows.size()-2; 
        i != -1 && supportRows[i].column == j; --i)
        mask &= supportRows[i].row;

    return mask;
}

template<typename Row>
bool RuncFixed<Row>::CreateChild( LocalState const& parent, int column, Row const& columnMask, LocalState& child)
{
    child.current = parent.current | columnMask;
    child.uncoveredRows.clear();
    child.supportRows.clear();
    child.candidates = 0;
    
    for (auto r : parent.uncoveredRows)
    {
        if (r & columnMask)
            child.supportRows.emplace_back(column, r);
        else
            child.uncoveredRows.push_back(r);
    }

    if (child.supportRows.empty())
        return false;

    if (child.uncoveredRows.empty())
        return true;

    child.compatable = parent.compatable 
        & ~GetLastSupportMask(child.supportRows);

    for (auto& sr: parent.supportRows)
    {
        int j = child.supportRows.back().column;
        if (j != column && j != sr.column)
            child.compatable &= ~GetLastSupportMask(child.supportRows);
        
        if (! (sr.row & columnMask))
            child.supportRows.push_back(sr);
    }

    if (child.supportRows.back().column != column && child.compatable)
        child.compatable &= ~GetLastSupportMask(child.supportRows);

    return true;
}

template<typename Row>
Row RuncFixed<Row>::FindRow( Rows const& uncoveredRows, Row const& mask) const
{
    Row minrow = 0;
    int count = sizeof(Row) * 8 + 1;

    for (auto r:uncoveredRows)
    {
        r &= mask;
        if (! r)
            return 0;
        int c = bits_count(r);            
        if (c < count)
        {
            count = c;
            minrow = r;
        }
    }

    return minrow;
}

template<typename Row>
RuncFixed<Row>::RuncFixed() 
    :innerCount(0), 
    extraCount(0), 
    resultsCount(0), 
    callbackDelay(100), 
    _callbackTick(0)
{
}

template<typename Row>
void RuncFixed<Row>::InitLocalStates( Rows rows, LocalStates& localStates )
{
    int m = rows.size();
    int q = sizeof(Row)*8;
    if (q > m)
        q = m;

    localStates.clear();
    localStates.reserve(q+2);

    auto& root = GetLocalState(localStates, 0);

    root.supportRows.clear();
    root.current = 0;
    root.uncoveredRows = std::move(rows);
    root.candidates = 0;
    root.compatable = 0;

    for (auto& r:root.uncoveredRows)
    {
        root.compatable |= r;
    }
}

template<typename Row>
bool RuncFixed<Row>::DoCallback()
{
    _callbackTick = 0;
    if (callback)
        return callback(this);
    return true;
}


template bits32 RuncFixedDualize<bits32>(std::vector<bits32> input, std::vector<bits32> input1);
template bits64 RuncFixedDualize<bits64>(std::vector<bits64> input, std::vector<bits64> input1);

template bits32 RuncFixedDualize<bits32>(std::vector<bits32> input);
template bits64 RuncFixedDualize<bits64>(std::vector<bits64> input);