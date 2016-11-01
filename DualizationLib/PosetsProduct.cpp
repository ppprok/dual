#include "stdafx.h"
#include "PosetsProduct.h"
#include "ReversedPoset.h"
#include "Registrator.h"
#include "PosetItemsMatrix.h"


void PosetsProduct::SetPosets( Posets posets, Strings bindings /*= Strings()*/ )
{
    _posets = std::move(posets);
    _reversedPosets.clear();
    _reversedPosets.reserve(_posets.size());
    for (auto& p : _posets)
        _reversedPosets.push_back(::Reverse(p));    
    _bindings = std::move(bindings);
}

Posets const& PosetsProduct::GetPosets() const
{
    return _posets;
}

Posets const& PosetsProduct::GetReversedPosets() const
{
    return _reversedPosets;
}

void PosetsProduct::SetOptions( Options const& options )
{
    Strings bindings;
    Posets posets;

    for(auto& p : options.get_child("columns"))
    {
        auto name = p.second.get("name", "");
        if (name.empty())
            continue;

        auto poset = Registrator<IPosetPtr>::Instance().Create(
            p.second.get<std::string>("type"));
        TrySetOptions(poset, p.second);
        posets.push_back(poset);
        bindings.push_back(name);                
    }

    SetPosets(std::move(posets), std::move(bindings));
}

void PosetsProduct::SetDefaultOptions()
{
    SetPosets(Posets());
}

/*bool PosetsProduct::Preceq( PosetItems const& left, PosetItems const& right ) const
{
    int n = _posets.size();
    assert(n == left.size() && n == right.size());

    for (int j = 0; j != n; ++j)
    {
        if (! _posets[j]->Preceq(left[j], right[j]))
            return false;
    }                
    return true;

}

bool PosetsProduct::Preceq( PosetItemsMatrix const& items, int rowId, PosetItems const& right ) const
{
    int n = _posets.size();
    assert(n == right.size());
    assert(n == 0 || inside(items[0], rowId));

    for (int j = 0; j != n; ++j)
    {
        if (! _posets[j]->Preceq(items[j][rowId], right[j]))
            return false;
    }                

    return true;

}

bool PosetsProduct::Preceq( PosetItems const& left, PosetItemsMatrix const& items, int rowId ) const
{
    int n = _posets.size();
    assert(n == left.size());
    assert(n == 0 || inside(items[0], rowId));

    for (int j = 0; j != n; ++j)
    {
        if (! _posets[j]->Preceq(left[j], items[j][rowId]))
            return false;
    }                

    return true;
}*/

bool PosetsProduct::IsIdealIndependent( PosetItemsMatrix const& idealBase, PosetItems const& x ) const
{
    assert(_posets.size() == x.size());

    for (int i = 0, m = idealBase.GetHeight(); i != m; ++i)
    {
        if (Preceq(idealBase[i], x))
            return false;
    }

    return true;

}

bool PosetsProduct::IsFilterIndependent( PosetItemsMatrix const& filterBase, PosetItems const& x ) const
{
    if (filterBase.GetHeight() == 0)
        return true;
    auto& pp = const_cast<PosetsProduct&>(*this);
    pp.Reverse();
    auto r = IsIdealIndependent(filterBase, x);
    pp.Reverse();
    return r;
}

bool PosetsProduct::IsMaximalIndependent( PosetItemsMatrix const& idealBase, PosetItems const& x ) const
{
    int n = _posets.size();
    assert(n == x.size());
    if (n == 0)
        return false;

    

    _supportMap.resize(n);

    for (auto& s: _supportMap)
        s.clear();

    for (int i = 0, m = idealBase.GetHeight(); i != m; ++i)
    {
        int j = CheckSupport(idealBase[i], x);
        if (j == -1)
            return false;
        if (j==-2)
            continue;
        _supportMap[j].push_back(idealBase[i][j]);
    }

    for (int j = 0; j != n; ++j)
    {
        if (_posets[j]->IsMaximal(x[j]))
            continue;
        if (_supportMap[j].empty())
            return false;            
        if (! _posets[j]->IsMaximalIndependent(x[j], _supportMap[j]))
            return false;
    }

    return true;

}

bool PosetsProduct::IsMinimalIndependent( PosetItemsMatrix const& filterBase, PosetItems const& x ) const
{
    auto& pp = const_cast<PosetsProduct&>(*this);
    pp.Reverse();
    auto r = IsMaximalIndependent(filterBase, x);
    pp.Reverse();
    return r;
}

bool PosetsProduct::IsIndependent( PosetItemsMatrix const& idealBase, PosetItemsMatrix const& filterBase, PosetItems const& lower, PosetItems const& upper ) const
{
    return Preceq(lower, upper) && IsIdealIndependent(idealBase, upper) &&
        IsFilterIndependent(filterBase, lower);
}

bool PosetsProduct::IsMaxMinIndependent( PosetItemsMatrix const& idealBase, PosetItemsMatrix const& filterBase, PosetItems const& lower, PosetItems const& upper ) const
{
    return Preceq(lower, upper) && IsMaximalIndependent(idealBase, upper) &&
        IsMinimalIndependent(filterBase, lower);
}

PosetItemsMatrix PosetsProduct::LoadItems( std::string const& filename, Options const& options )
{
    std::string separators = options.get("separators", ",");
    std::string spaces = options.get("spaces", " \t\r");
    std::string eol = options.get("eol", "\n");

    CSVLoader csv;
    csv.Load(filename.c_str());
    csv.CreateColumns(CharMap(separators), CharMap(eol));
    csv.TrimSpaces(CharMap(spaces));
        

    int n = _posets.size();
    int m = csv.GetColumnsHeight();
    if (m > 0)
        --m;

    PosetItemsMatrix items;
    items.Resize(m, n);

    for (int j = 0; j != n; ++j)
    {
        auto& csvColumn = csv.GetColumn(_bindings[j]);
        auto column = items.GetColumn(j);
        for (int i = 0; i != m; ++i)
        {
            column[i] = _posets[j]->FromString(
            csvColumn.values[i+1].begin, 
            csvColumn.values[i+1].end);
        }
    }

    return items;

}

void PosetsProduct::Reverse()
{
    std::swap(_posets, _reversedPosets);
}

int PosetsProduct::CheckSupport(PosetItemsMatrix::CRowRef const& idealBaseRow, PosetItems const& x ) const
{
    int n = _posets.size();
    assert(n == x.size());
    //assert(n == 0 || inside(idealBase[0], i));

    int l = -1; // not independent
    for (int j = 0; j != n; ++j)
    {
        if (! _posets[j]->Preceq(idealBaseRow[j], x[j]))
        {
            if (l >= 0)
                return -2; // not support
            l = j;
        }
    }
    return l;

}


void PosetsProduct::GetSupportItems( PosetItemsMatrix const& dataset, PosetItems const& items, std::vector<int>& support ) const
{
    support.clear();
    for (int i = 0, m = dataset.GetHeight(); i != m; ++i)
        if (Preceq(items, dataset[i]))
            support.push_back(i);
}

int PosetsProduct::GetPosetIndex( std::string const& name ) const
{
    auto beg = _bindings.begin(), end = _bindings.end(), i = std::find(beg, end, name);
    if (i == end)
        BOOST_THROW_EXCEPTION(error("Poset isn't founded by name") << column_name(name));
    return i - beg;
}

std::string PosetsProduct::ToString( PosetItemsMatrix const& matrix, std::string const& separator /*= ","*/, std::string rowSep /*= "\n"*/ ) const
{
    std::ostringstream oss;

    for (int i = 0, m = matrix.GetHeight(), n = matrix.GetWidth(); i != m; ++i)
    {
        if (i)
            oss << rowSep;
        auto row = matrix[i];
        for (int j = 0; j != n; ++j)
        {
            if (j)
                oss << separator;
            oss << _posets[j]->ToString(row[j]);
        }
    }

    return oss.str();
}

void PosetsProduct::GetUnsupportItems( PosetItemsMatrix const& dataset, PosetItems const& items, std::vector<int>& unsupport ) const
{
    unsupport.clear();
    for (int i = 0, m = dataset.GetHeight(); i != m; ++i)
        if (! Preceq(items, dataset[i]))
            unsupport.push_back(i);
}

void PosetsProduct::UpdateUnsupportItems( PosetItemsMatrix const& dataset, PosetItems const& items, std::vector<int>& unsupport ) const
{
    remove_if(unsupport, [this, &items, &dataset](int i)
        {return Preceq(items, dataset[i]);});
}

void PosetsProduct::UpdateSupportItems( PosetItemsMatrix const& dataset, PosetItems const& items, std::vector<int>& support ) const
{
    remove_if(support, [this, &items, &dataset](int i)
    {return ! Preceq(items, dataset[i]);});
}



/*void PosetsProduct::ToString( PosetItems const& item, std::string const& separator ) const
{

}*/