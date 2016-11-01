#include "stdafx.h"
#include "Runc.h"
#include "DualizationBacktrackAlgorithmBase.h"
#include "BoolMatrixPreparation.h"


// Инициализировать по глобальному состоянию
void Runc::LocalState::Init( GlobalState& g )
{
    int m = g._matrix.height();
    int n = g._matrix.width();    
    
    _uncovered.clear();
    _support.clear();
    _supportGroups.clear();
    
    _uncovered.reserve(m);
    _support.reserve(m);
    _supportGroups.reserve(std::min(m, n));    
    _restCols = bit_vector(n);
}

// Установить опорные строки
bool Runc::LocalState::SetUncoveredSupportRows( 
    GlobalState& global, 
    int col,
    marked_ranges const& groups, 
    rows const& support)
{
    
    for (auto g: groups)		
    {
        bool b = AddUncoveredSupportRows(global, col, g.get_value(), 
            g.begin_in(support), g.end_in(support));
        if (! b)
            return false;
    }

    return true;
}

bool Runc::LocalState::SetUncoveredSupportRows( GlobalState& g, LocalState const& parent, int col )
{
    return SetUncoveredSupportRows(g, col, parent._supportGroups, parent._support);
}


// Добавить группу опорных строк
bool Runc::LocalState::AddUncoveredSupportRows( 
    GlobalState& global, 
    int col,
    int l, 
    rows_citerator begin, 
    rows_citerator end)
{
    int s = _support.size();
    auto& tempRow = global._tempRow;
    tempRow.assign(GetRestCols());

    for (; begin != end; ++begin)
    {
        if (col >= 0 && begin->get(col))
            continue;

        auto r = *begin;
        
        if (! tempRow.empty())
            tempRow.assign_and(r, tempRow);

        if (tempRow.empty())
        {
            GetRestCols().trim(r);
            if (r.empty())
            {
                _support.resize(s);
                return true;
            }
        }

        _support.push_back(r);
    }

    if (s == _support.size())
        return false;

    if (! tempRow.empty())
        _restCols.assign_sub(_restCols, tempRow);

    _supportGroups.push_back(
        marked<range>(range(s, _support.size()), l));

    return true;
}

// Установить множество непокрытых строк
bool Runc::LocalState::SetUncovered(
    GlobalState& g, int col, rows_citerator begin, rows_citerator end )
{
    _uncovered.resize(end - begin);
    auto uend = _uncovered.end();
    auto ubeg = _uncovered.begin();

    for(; begin != end; ++begin)
    {
        if (begin->get(col))
        {
            *--uend = *begin;
            continue;
        }

        auto r = *begin;						
        
        GetRestCols().trim(r);

        if (r.empty())
            return false;
                
        *ubeg++ = r;
    }

    if (! AddUncoveredSupportRows(g, -1, col, uend, _uncovered.end()))
        return false;

    _uncovered.resize(ubeg - _uncovered.begin());
    return true;
}



bool Runc::LocalState::SetUncovered( GlobalState& g, LocalState const& parent, int col )
{
    return SetUncovered(g, col, 
        parent._uncovered.begin(), 
        parent._uncovered.end());
}



// Добавить столбец
Runc::LocalState::AddResult 
    Runc::LocalState::AddColumn( GlobalState& g, LocalState const& l, int col)
{
    _support.clear();
    _supportGroups.clear();
    _uncovered.clear();

    _restCols.assign(l._restCols);
    
    if (! SetUncovered(g, l, col))
        return Uncoverable;

    if (! SetUncoveredSupportRows(g, l, col))
        return Uncompatable;
    
    return IsCover() ? Cover : Inner;
}



// Удалить столбец
void Runc::LocalState::RemoveColumn( int j )
{
    _restCols.set(j, 0);
}



// Найти минимальную непокрытую строку
bit_chunk Runc::LocalState::GetMinUncovered( int mc /*= 0*/ ) const
{
    bit_chunk row;
    int count = INT_MAX;

    for(auto r : _uncovered)
    {
        int c = GetRestCols().count(r, count);

        if (c < count)
        {
            if (c <= mc)
                return r;

            count = c;
            row = r;
        }
    }

    return row;
}

// Удалить нулевые столбцы
bit_chunk Runc::LocalState::CollectRestCols()
{
    _restCols.clear();

    for(auto r : _uncovered)
        _restCols.assign_or(_restCols, r);

    return _restCols;
}


// Удалить охватывающие строки
void Runc::LocalState::RemoveDependentRows()
{
    BoolMatrixPreparation::RemoveDependentRows(_uncovered, GetRestCols());
}

bit_chunk Runc::LocalState::GetUndepCols(GlobalState& g)
{
    RemoveDependentRows();

    auto& tempRow = g._tempRow;
    tempRow.clear();
    for(auto r:_uncovered)
    {
        tempRow.assign_or(r, tempRow);
    }
    _restCols.assign_and(_restCols, tempRow);

    return _restCols;
}

bit_chunk Runc::LocalState::GetCols(GlobalState& g,  Runc::AlgorithmType alg )
{
    switch (alg)
    {
    case Runc::OPT:
        return GetUndepCols(g);
    case Runc::RUNC:
        return GetFrontUncovered();
    case Runc::RUNCM:
        return GetMinUncovered(1);
    }
    return bit_chunk();
}

bit_chunk Runc::LocalState::GetFrontUncovered() const
{
    return _uncovered.front();
}

bit_mask Runc::LocalState::GetRestCols() const
{
    return _restCols;
}

bool Runc::LocalState::IsCover() const
{
    return _uncovered.empty();
}


Runc::LocalState::LocalState()
{
}


bool Runc::GlobalState::DoDualize(int stateNum)
{
    auto current =  GetLocalState(stateNum);

    if (! current)
        return true;

    auto cols = current->GetCols(*this, _type);

    while(! _currentNode.stoped)
    {
        int j = current->GetRestCols().find_last(cols);
        
        if (j < 0)
            break;
		
		current->RemoveColumn(j);

		if (AddColumn(j))
		{
			
			auto child = GetLocalState(stateNum + 1);
			auto r = child->AddColumn(*this, *current, j);

			switch (r)
			{
			case Runc::LocalState::Inner:
				if  (! DoDualize(stateNum + 1))
					return false;
				break;
			case Runc::LocalState::Cover:
				DoCoverCallback();
				break;				
			}
		}
		Backtrack();
    }
	
    return ! _currentNode.stoped;
}



Runc::LocalState* Runc::GlobalState::GetLocalState(int stateNum)
{
    while (stateNum >= (int)_states.size())
        _states.emplace_back(*this);

    return &_states[stateNum];
}


void Runc::GlobalState::Dualize()
{
    int n = _matrix.width();
    int m = _matrix.height();

    int q = std::min(m, n);

    if (q <= 0)
        return;
        
    _states.clear();
    _states.reserve(q+1);

    _tempRow = bit_vector(n);
    
    auto root = GetLocalState(0);
    root->_uncovered.assign(_matrix.get_rows().begin(), _matrix.get_rows().end());
    root->CollectRestCols();
	    
    ResetCurrentNode(q);
    
    DoDualize(0);
}

Runc::GlobalState::GlobalState( AlgorithmType type /*= AlgorithmType::RUNCM*/ ) 
    :_type(type)
{

}
