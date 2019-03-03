
#include "Antichain.h"
#include "Registrator.h"

void Antichain::SetOptions( Options const& options )
{
    _includeUniversalMaximal = options.get("IncludeUniversalMaximal", false);
    _includeUniversalMinimal = options.get("IncludeUniversalMinimal", false);
    _size = options.get("Size", 2);
}

void Antichain::SetDefaultOptions()
{
    _includeUniversalMinimal = false;
    _includeUniversalMaximal = false;
    _size = 2;
}

void Antichain::Register()
{
    Registrator<IPosetPtr>::Instance().RegisterFactory
        ("Antichain", [](){return std::make_shared<Antichain>();});
}

bool Antichain::Equal( PosetItem const& left, PosetItem const& right ) const
{
    return left.value.vertex == right.value.vertex;
}

bool Antichain::Prec( PosetItem const& left, PosetItem const& right ) const
{
    if (left.value.vertex == right.value.vertex)
        return false;

    return IsUniversalMaximal(right) || IsUniversalMinimal(left);
}

bool Antichain::Preceq( PosetItem const& left, PosetItem const& right ) const
{
    if (IsUniversalMaximal(right) || IsUniversalMinimal(left))
        return true;

    return left.value.vertex == right.value.vertex;
}


bool Antichain::IsMaximal( PosetItem const& a ) const
{
    if (_includeUniversalMaximal)
        return IsUniversalMaximal(a);

    return ! IsUniversalMinimal(a) && IsItem(a);
}

bool Antichain::IsMinimal( PosetItem const& a ) const
{
    if (_includeUniversalMinimal)
        return IsUniversalMinimal(a);

    return ! IsUniversalMaximal(a) && IsItem(a);
}

void Antichain::SelectUnique( PosetItems& items ) const
{
    TopologicalSort(items);
    auto end = items.end();
    auto pos = std::unique(items.begin(), end, 
        [](PosetItem const& a, PosetItem const& b)
        {return a.value.vertex == b.value.vertex;});
    items.erase(pos, end);
}

void Antichain::TopologicalSort( PosetItems& items ) const
{
    sort(items, [](PosetItem const& a, PosetItem const& b)
        {return a.value.vertex > b.value.vertex;});
}

bool Antichain::IsItem( PosetItem const& a ) const
{
    if (IsUniversalMinimal(a))
        return _includeUniversalMinimal;

    if (IsUniversalMaximal(a))
        return _includeUniversalMaximal;

    return a.value.vertex >=0 && a.value.vertex < _size;
}

bool Antichain::IsMaximalIndependent( PosetItem const& a, PosetItems const& A ) const
{
    if (! IsItem(a))
        return false;

    if (InIdeal(a, A))
        return false;

    if (IsUniversalMinimal(a))
    {
        for (int i = 0; i < _size; ++i)
            if (! contains(A, PosetItem(i)))
                return false;
    }
    
    return true;
}

bool Antichain::IsMinimalIndependent( PosetItem const& a, PosetItems const& B ) const
{
    if (! IsItem(a))
        return false;

    if (InFilter(a, B))
        return false;

    if (IsUniversalMaximal(a))
    {
        for (int i = 0; i < _size; ++i)
            if (! contains(B, PosetItem(i)))
                return false;
    }

    return true;
}

void Antichain::GetMaximal( PosetItems& maximal ) const
{
    if (_includeUniversalMaximal)
        return maximal.assign(1, GetUniversalMaximal());
    
    maximal.clear();        
    for (int i = 0; i < _size; ++i)
        maximal.emplace_back(i);    
}

void Antichain::GetMinimal( PosetItems& minimal ) const
{
    if (_includeUniversalMinimal)
        return minimal.assign(1, GetUniversalMinimal());

    minimal.clear();        
    for (int i = 0; i < _size; ++i)
        minimal.emplace_back(i);
}

std::string Antichain::ToString( PosetItem const& item ) const
{
    if (IsUniversalMaximal(item))
        return "+\\infty";

    if (IsUniversalMinimal(item))
        return "-\\infty";

    return boost::lexical_cast<std::string>(item.value.vertex);
}

PosetItem Antichain::FromString( char const* begin, char const* end ) const
{
    
    if (streq(begin, end,"+\\infty"))
        return GetUniversalMaximal();

    if (streq(begin, end,"-\\infty"))
        return GetUniversalMinimal();

    return boost::lexical_cast<int>(begin, end - begin);
}

bool Antichain::IsUniversalMinimal( PosetItem const& item ) const
{
    return item.value.vertex == std::numeric_limits<int>::min();
}

bool Antichain::IsUniversalMaximal( PosetItem const& item ) const
{
    return item.value.vertex == std::numeric_limits<int>::max();
}

void Antichain::GetAllItems( PosetItems& items ) const
{
    int n = _size;
    if (_includeUniversalMaximal)
        ++n;
    if (_includeUniversalMinimal)
        ++n;
    items.reserve(n);
    if (_includeUniversalMinimal)
        items.push_back(GetUniversalMinimal());
    for (int i = 0; i < _size; ++i)
        items.emplace_back(i);
    if (_includeUniversalMaximal)
        items.push_back(GetUniversalMaximal());

}

PosetItem Antichain::GetUniversalMaximal() const
{
    return std::numeric_limits<int>::max();
}

PosetItem Antichain::GetUniversalMinimal() const
{
    return std::numeric_limits<int>::min();
}

bool Antichain::PrecImmediate( PosetItem const& a, PosetItem const& b ) const
{
    if (_size > 0)
    {
        return (IsUniversalMinimal(a) && ! IsUniversalMaximal(b)) ||
            (IsUniversalMaximal(a) && ! IsUniversalMinimal(b));
    }
    
    return IsUniversalMinimal(a) && IsUniversalMaximal(b);
}

void Antichain::GetImmediatePrec( PosetItem const& item, PosetItems& items ) const
{
    items.clear();
    if (IsUniversalMinimal(item))
        return;

    if (IsUniversalMaximal(item))
    {
        for (int i =0 ; i < _size; ++i)
            items.emplace_back(i);
        if (_includeUniversalMinimal && items.empty())
            items.push_back(GetUniversalMinimal());
        return;
    }

    if (IsItem(item) && _includeUniversalMinimal)
        items.push_back(GetUniversalMinimal());
}

void Antichain::GetImmediateSucc( PosetItem const& item, PosetItems& items ) const
{
    items.clear();
    if (IsUniversalMaximal(item))
        return;

    if (IsUniversalMinimal(item))
    {
        for (int i =0 ; i < _size; ++i)
            items.emplace_back(i);
        if (_includeUniversalMaximal && items.empty())
            items.push_back(GetUniversalMaximal());
        return;
    }

    if (IsItem(item) && _includeUniversalMaximal)
        items.push_back(GetUniversalMaximal());
}

Antichain::Antichain()
{
    SetDefaultOptions();
}

void Antichain::GetMinUpperBounds(PosetItems const &items, PosetItems &upperBounds) const {

}

void Antichain::GetMaxLowerBounds(PosetItems const &items, PosetItems &lowerBounds) const {

}
