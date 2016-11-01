#include "stdafx.h"

#include "PosetBase.h"
#include "containers.h"

bool PosetBase::Prec( PosetItem const& left, PosetItem const& right ) const
{
    if (left == right)
        return false;

    return Preceq(left, right) && ! Preceq(right, left);
}

bool PosetBase::Equal( PosetItem const& left, PosetItem const& right ) const
{
    if (left == right)
        return true;

    return Preceq(left, right) && Preceq(right, left);
}

void PosetBase::SelectUnique( PosetItems& items ) const
{
    unique(items);
    
    auto beg = items.begin(), end = items.end();
    for(; beg != end; ++beg)
    {
        auto l = *beg;
        end = std::partition(beg+1, end, [this, l](PosetItem const& r){ 
            return ! Equal(l, r);});
    }
    
    items.erase(end, items.end());
}

void PosetBase::SelectFilter( PosetItems const& A, PosetItems& B ) const
{
    auto end = B.end();
    auto i = std::remove_if(B.begin(), end, 
        [&A, this](PosetItem const& a){return ! InFilter(a, A);});

    B.erase(i, end);
}

void PosetBase::SelectIdeal( PosetItems const& A, PosetItems& B ) const
{
    auto end = B.end();
    auto i = std::remove_if(B.begin(), end, 
        [&A, this](PosetItem const& a){return ! InIdeal(a, A);});

    B.erase(i, end);
}

void PosetBase::SelectIdealIndependent( PosetItems const& A, PosetItems& B ) const
{
    auto end = B.end();
    auto i = std::remove_if(B.begin(), end, 
        [&A, this](PosetItem const& a){return InIdeal(a, A);});
    B.erase(i, end);
}

void PosetBase::SelectMaximal( PosetItems& A ) const
{
    auto end = A.end();
    auto i = std::remove_if(A.begin(), end, 
        [&A, this](PosetItem const& a){return ! IsMaximal(a, A);});

    A.erase(i, end);
}

bool PosetBase::InFilter( PosetItem const& a, PosetItems const& A ) const
{
    for (auto& b : A)
        if (Preceq(a, b))
            return true;
    return false;
}

bool PosetBase::InIdeal( PosetItem const& a, PosetItems const& A ) const
{
    for (auto& b : A)
        if (Preceq(b, a))
            return true;
    return false;
}

bool PosetBase::IsIndependent( PosetItem const& a, PosetItems const& A, PosetItems const& B ) const
{
    return ! InIdeal(a, A) && ! InFilter(a, B);
}

bool PosetBase::IsMaximal( PosetItem const& a, PosetItems const& A ) const
{
    for(auto& b : A)
        if (Prec(a, b))
            return false;
    return true;
}

bool PosetBase::IsMaximal( PosetItem const& a ) const
{
    if (! IsItem(a))
        return false;
    
    GetImmediateSucc(a, _temp);
    return _temp.empty();
}

bool PosetBase::IsMinimal( PosetItem const& a, PosetItems const& A ) const
{
    for(auto& b : A)
        if (Prec(b, a))
            return false;
    return true;
}

bool PosetBase::IsMinimal( PosetItem const& a ) const
{
    if (! IsItem(a))
        return false;
    
    GetImmediatePrec(a, _temp);
    return _temp.empty();
}

void PosetBase::SelectMinimal( PosetItems& A ) const
{
    auto end = A.end();
    auto i = std::remove_if(A.begin(), end, 
        [&A, this](PosetItem const& a){return ! IsMinimal(a, A);});

    A.erase(i, end);
}

void PosetBase::SelectFilterIndependent( PosetItems const& A, PosetItems& B ) const
{
    auto end = B.end();
    auto i = std::remove_if(B.begin(), end, 
        [&A, this](PosetItem const& a){return InFilter(a, A);});
    B.erase(i, end);
}

void PosetBase::SelectIndependent( PosetItems const& A, PosetItems const& B, PosetItems& X ) const
{
    if (! X.empty())
        SelectIdealIndependent(A, X);
    if (! X.empty())
        SelectFilterIndependent(B, X);
}

void PosetBase::GetMaximal( PosetItems& maximal ) const
{
    if (IsItem(GetUniversalMaximal()))
        return maximal.assign(1, GetUniversalMaximal());
    
    GetImmediatePrec(GetUniversalMaximal(), maximal);    
}

void PosetBase::GetMinimal( PosetItems& minimal ) const
{
    if (IsItem(GetUniversalMinimal()))
        return minimal.assign(1, GetUniversalMinimal());

    GetImmediateSucc(GetUniversalMinimal(), minimal);
}

void PosetBase::GetMaximalIndependent( PosetItems const& A, PosetItems& maximal ) const
{
    GetAllItems(maximal);
    SelectIdealIndependent(A, maximal);
    SelectMaximal(maximal);
}

void PosetBase::GetMinimalIndependent( PosetItems const& A, PosetItems& minimal ) const
{
    GetAllItems(minimal);
    SelectFilterIndependent(A, minimal);
    SelectMinimal(minimal);
}

PosetItem PosetBase::FromStdString( std::string const& s ) const
{
    auto cs = s.c_str();
    return FromString(cs, cs + s.length());
}

bool PosetBase::IsMaximalIndependent( PosetItem const& a, PosetItems const& A ) const
{
    if (InIdeal(a, A))
        return false;
    
    if (A.empty())
        return IsMaximal(a);

    if (! IsItem(a))
        return false;
        
    GetImmediateSucc(a, _temp);
        
    for (auto& b : _temp)
        if (! InIdeal(b, A))
            return false;
    
    return true;
}

bool PosetBase::IsMinimalIndependent( PosetItem const& a, PosetItems const& B ) const
{
    if (InFilter(a, B))
        return false;

    if (B.empty())
        return IsMinimal(a);

    if (! IsItem(a))
        return false;

    GetImmediatePrec(a, _temp);

    for (auto& b : _temp)
        if (! InFilter(b, B))
            return false;

    return true;
}

void PosetBase::GetIndependentIntervals( PosetItems const& A, PosetItems const& B, PosetIntervals& independent ) const
{
    PosetItems maximal, minimal;
    GetMaximalIndependent(A, maximal);
    GetMinimalIndependent(B, minimal);
    
    independent.clear();

    for (auto& y:maximal)
        for (auto& x:minimal)
            if (Preceq(x, y))
                independent.emplace_back(x, y);
}


void PosetBase::TopologicalSort( PosetItems& items ) const
{
    int n = items.size();
    items.reserve(n*3);
    items.resize(n*2, PosetItem(0));

    for (int i = 0, t = n; i != n; ++i, ++t)
        if (items[t].value.vertex == 0)
            DoTopologicalSort(items, n, i);

    for (int i = 0, t = 2*n; i != n; ++i, ++t)
        items[i] = items[t];

    items.resize(n);
}

void PosetBase::DoTopologicalSort( PosetItems& items, int n, int i) const
{
    items[i+n].value.vertex=1;
    for (int t = 0; t != n; ++t)
    {
        if (t == i || items[n+t].value.vertex != 0)
            continue;

        if (Preceq(items[i], items[t]))
            DoTopologicalSort(items, n, t);        
    }

    items.push_back(items[i]);
}

bool PosetBase::IsTopologicalOrdered( PosetItems const& items ) const
{
    if (items.empty())
        return true;
    
    for (int n = items.size(), i = 0; i != n-1; ++i)
        for (int j = i+1; j != n; ++j)
        {
            if (Prec(items[i], items[j]))
                return false;
        }

    return true;
}

bool PosetBase::IsUniversalMaximal( PosetItem const& item ) const
{
    return Equal(GetUniversalMaximal(), item);
}

bool PosetBase::IsUniversalMinimal( PosetItem const& item ) const
{
    return Equal(GetUniversalMinimal(), item);
}