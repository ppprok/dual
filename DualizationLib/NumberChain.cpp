#include "stdafx.h"

#include "containers.h"
#include "CSVLoader.h"
#include "NumberChain.h"

/*NumberChain::NumberChain() 
{
    SetDefaultOptions();
}

void NumberChain::SetOptions( Options const& options )
{
    SetDefaultOptions();
    //_epsilon = options.get("Epsilon", _epsilon);
}

void NumberChain::SetDefaultOptions()
{
    //_epsilon = 0.00001;
}*/

void NumberChain::Register()
{
    Registrator<IPosetPtr>::Instance().RegisterFactory
        ("NumberChain", [](){return std::make_shared<NumberChain>();});
}

bool NumberChain::Prec( PosetItem const& left, PosetItem const& right ) const
{
    return left.value.dnumber < right.value.dnumber;
}

bool NumberChain::Equal( PosetItem const& left, PosetItem const& right ) const
{
    return left.value.dnumber == right.value.dnumber;
}

bool NumberChain::Preceq( PosetItem const& left, PosetItem const& right ) const
{
    return left.value.dnumber <= right.value.dnumber;
}

void NumberChain::GetMaximal( PosetItems& maximal ) const
{
    maximal.assign(1, GetUniversalMaximal());    
}

void NumberChain::GetMinimal( PosetItems& minimal ) const
{
    minimal.assign(1, GetUniversalMinimal());
}

PosetItem NumberChain::GetUniversalMaximal() const
{
    return std::numeric_limits<PosetItem::ChainNumber>::max();
}

PosetItem NumberChain::GetUniversalMinimal() const
{
    return -std::numeric_limits<PosetItem::ChainNumber>::max();
}

NumberChain::Number NumberChain::GetMaximalNumber( PosetItems const& A ) const
{
    if (A.empty())
        return -std::numeric_limits<PosetItem::ChainNumber>::max();

    auto m = A.front().value.dnumber;

    for (auto b = A.begin()+1, e = A.end(); b != e; ++b)
    {
        if (m < b->value.dnumber)
            m = b->value.dnumber;
    }

    return m;
}

PosetItem::ChainNumber NumberChain::GetMinimalNumber( PosetItems const& A ) const
{
    if (A.empty())
        return std::numeric_limits<Number>::max();

    auto m = A.front().value.dnumber;

    for (auto b = A.begin()+1, e = A.end(); b != e; ++b)
    {
        if (m > b->value.dnumber)
            m = b->value.dnumber;
    }

    return m;
}


bool NumberChain::IsItem( PosetItem const& a ) const
{
    return true;
}

bool NumberChain::IsMaximalIndependent( PosetItem const& a, PosetItems const& A ) const
{
    if (A.empty())
        return IsMaximal(a);
    return PrecImmediate(a, GetMinimalNumber(A));
}

bool NumberChain::IsMinimalIndependent( PosetItem const& a, PosetItems const& B ) const
{
    if (B.empty())
        return IsMinimal(a);
    return PrecImmediate(GetMaximalNumber(B), a);
}


std::string NumberChain::ToString( PosetItem const& item ) const
{
    if (IsMaximal(item))
        return "+\\infty";

    if (IsMinimal(item))
        return "-\\infty";

    return boost::lexical_cast<std::string>(item.value.dnumber);
}

PosetItem NumberChain::FromString( char const* begin, char const* end ) const
{
    if (streq(begin, end, "+\\infty"))
        return GetUniversalMaximal();

    if (streq(begin, end, "-\\infty"))
        return GetUniversalMinimal();

    return boost::lexical_cast<Number>(std::string(begin, end));
}


void NumberChain::SelectUnique( PosetItems& items ) const
{
    sort(items, [](PosetItem const& a, PosetItem const& b){return a.value.dnumber < b.value.dnumber;});
    unique(items, true);
}

void NumberChain::SelectFilter( PosetItems const& A, PosetItems& B ) const
{
    if (B.empty())
        return;
    
    if (A.empty())
    {
        B.clear();
        return;
    }


    auto maximal = GetMaximalNumber(A);
    auto end = B.end();
    B.erase(std::remove_if(B.begin(), end, 
        [maximal](PosetItem const b){return b.value.dnumber > maximal;}), end);
}


void NumberChain::SelectIdeal( PosetItems const& A, PosetItems& B ) const
{
    if (B.empty())
        return;
    
    if (A.empty())
    {
        B.clear();
        return;
    }

    auto minimal = GetMinimalNumber(A);    
    auto end = B.end();
    B.erase(std::remove_if(B.begin(), end, 
        [minimal](PosetItem const b){return b.value.dnumber < minimal;}), end);
}

void NumberChain::SelectFilterIndependent( PosetItems const& A, PosetItems& B ) const
{
    if (B.empty() || A.empty())
        return;

    auto maximal = GetMaximalNumber(A);    
    auto end = B.end();
    B.erase(std::remove_if(B.begin(), end, 
        [maximal](PosetItem const b){return b.value.dnumber <= maximal;}), end);
}

void NumberChain::SelectIdealIndependent( PosetItems const& A, PosetItems& B ) const
{
    if (B.empty() || A.empty())
        return;

    auto minimal = GetMinimalNumber(A);    
    auto end = B.end();
    B.erase(std::remove_if(B.begin(), end, 
        [minimal](PosetItem const b){return b.value.dnumber >= minimal;}), end);
}



void NumberChain::SelectIndependent( PosetItems const& A, PosetItems const& B, PosetItems& X ) const
{
    if (X.empty())
        return;

    if (A.empty())
        return SelectFilterIndependent(B, X);
    
    if (B.empty())
        return SelectIdealIndependent(A, X);

    auto maximal = GetMaximalNumber(B);
    auto minimal = GetMinimalNumber(A);

    auto end = X.end();
    X.erase(std::remove_if(X.begin(), end, 
        [minimal, maximal](PosetItem const b){
            return b.value.dnumber >= minimal || b.value.dnumber <= maximal;}), end);


}

void NumberChain::SelectMaximal( PosetItems& A ) const
{
    if (A.empty())
        return;

    A.assign(1, GetMaximalNumber(A));    
}

void NumberChain::SelectMinimal( PosetItems& A ) const
{
    if (A.empty())
        return;

    A.assign(1, GetMinimalNumber(A));
}


bool NumberChain::IsMaximal( PosetItem const& a ) const
{
    return a == GetUniversalMaximal();
}

bool NumberChain::IsMinimal( PosetItem const& a ) const
{
    return a == GetUniversalMinimal();
}

void NumberChain::TopologicalSort( PosetItems& items ) const
{
    sort(items, [](PosetItem const& i1, PosetItem const& i2){return i1.value.dnumber > i2.value.dnumber;});
}


void NumberChain::GetMaximalIndependent( PosetItems const& A, PosetItems& maximal ) const
{
    if (A.empty())
        return GetMaximal(maximal);
        
    auto min = GetMinimalNumber(A);
    if (IsMinimal(min))
        return maximal.clear();
    maximal.assign(1, min - 1);
}


void NumberChain::GetMinimalIndependent( PosetItems const& B, PosetItems& minimal ) const
{
    if (B.empty())
        return GetMinimal(minimal);
        
    auto max = GetMaximalNumber(B);
    if (IsMaximal(max))
        return minimal.clear();
    
    minimal.assign(1, max + 1);
}

void NumberChain::GetAllItems( PosetItems& items ) const
{
    BOOST_THROW_EXCEPTION(too_many_items_error());
}

void NumberChain::GetIndependentIntervals( PosetItems const& A, PosetItems const& B, PosetIntervals& independent ) const
{
    if (A.empty() && B.empty())
        return independent.assign(1, PosetInterval(GetUniversalMinimal(), GetUniversalMaximal()));
    
    auto upper = GetMinimalNumber(A);
    auto lower = GetMaximalNumber(B);

    if (lower >= upper)
        return independent.clear();

    --upper;
    ++lower;

    if (lower > upper)
        return independent.clear();

    independent.assign(1, PosetInterval(PosetItem(lower), PosetItem(upper)));
}

bool NumberChain::IsTopologicalOrdered( PosetItems const& items ) const
{
    return std::is_sorted(items.begin(), items.end(), 
        [](PosetItem const& i1, PosetItem const& i2)
    {return i1.value.dnumber > i2.value.dnumber;});
}

bool NumberChain::PrecImmediate( PosetItem const& a, PosetItem const& b ) const
{
    return a.value.dnumber < b.value.dnumber && a.value.dnumber+1 == b.value.dnumber;
}

void NumberChain::GetImmediatePrec( PosetItem const& item, PosetItems& items ) const
{
    if (IsMinimal(item))
        return items.clear();
    items.assign(1, item.value.dnumber-1);
}

void NumberChain::GetImmediateSucc( PosetItem const& item, PosetItems& items ) const
{
    if (IsMaximal(item))
        return items.clear();
    items.assign(1, item.value.dnumber+1);
}

void NumberChain::GetMinUpperBounds( PosetItems const& items, PosetItems& upperBounds ) const
{
    if (items.empty())    
        return upperBounds.assign(1, GetUniversalMinimal());
    upperBounds.assign(1, GetMaximalNumber(items));
}

void NumberChain::GetMaxLowerBounds( PosetItems const& items, PosetItems& lowerBounds ) const
{
    if (items.empty())    
        return lowerBounds.assign(1, GetUniversalMaximal());
    lowerBounds.assign(1, GetMinimalNumber(items));
}