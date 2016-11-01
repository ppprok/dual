#include "stdafx.h"

#include "IntervalLattice.h"

#include "errors.h"

void IntervalLattice::Register()
{
    Registrator<IPosetPtr>::Instance().RegisterFactory
        ("IntervalLattice", [](){return std::make_shared<IntervalLattice>();});
}

bool IntervalLattice::Equal( PosetItem const& left, PosetItem const& right ) const
{
    auto l1 = GetLength(left);
    if (l1 < 0)
        return false;
    auto l2 = GetLength(right);
    if (l1 != l2)
        return false;
    if (l1 == 0)
        return true;

    return left.value.interval.min == right.value.interval.min &&
        left.value.interval.max == right.value.interval.max;
}

bool IntervalLattice::Prec( PosetItem const& left, PosetItem const& right ) const
{
    auto l1 = GetLength(left);
    if (l1 < 0)
        return false;
    auto l2 = GetLength(right);
    if (l1 >= l2)
        return false;
    
    return right.value.interval.min <= left.value.interval.min && 
           left.value.interval.max <= right.value.interval.max;
}

bool IntervalLattice::Preceq( PosetItem const& left, PosetItem const& right ) const
{
    auto l1 = GetLength(left);
    if (l1 < 0)
        return false;
    auto l2 = GetLength(right);
    if (l2 < 0)
        return false;
    if (l1 == 0)
        return true;
    if (l2 == 0)
        return l1 == 0;
    
    return right.value.interval.min <= left.value.interval.min && 
        left.value.interval.max <= right.value.interval.max;
}

void IntervalLattice::GetMaximal( PosetItems& maximal ) const
{
    maximal.assign(1, GetUniversalMaximal());
}

PosetItem IntervalLattice::GetUniversalMaximal() const
{
    return _universalMaximal;
}

bool IntervalLattice::IsMinimal( PosetItem const& a ) const
{
    return a.value.interval.min == a.value.interval.max;
}

PosetItem IntervalLattice::GetUniversalMinimal() const
{
    return PosetItem(0, 0);
}

bool IntervalLattice::IsMaximal( PosetItem const& a ) const
{
    return a == _universalMaximal;
}

bool IntervalLattice::IsItem( PosetItem const& a ) const
{
    return Preceq(a, _universalMaximal);
}

void IntervalLattice::GetMinimal( PosetItems& minimal ) const
{
    minimal.assign(1, GetUniversalMinimal());
}

void IntervalLattice::GetAllItems( PosetItems& items ) const
{
    int n = GetLength(_universalMaximal);
    items.reserve(n*(n+1)/2+1);

    for (int i = _universalMaximal.value.interval.min; 
        i < _universalMaximal.value.interval.max; ++i)
    {
        for (int j = i+1; 
            j <= _universalMaximal.value.interval.max; ++j)
        {
            items.emplace_back(i,j);
        }
    }
    
    items.push_back(GetUniversalMinimal());
}

std::string IntervalLattice::ToString( PosetItem const& item ) const
{
    if (IsMinimal(item))
        return "\\emptyset";

    return '[' + ToString(item.value.interval.min) + ':' + ToString(item.value.interval.max)  + ']';
}

std::string IntervalLattice::ToString( PosetItem::IntervalBound a ) const
{
    /*const auto inf = std::numeric_limits<PosetItem::IntervalBound>::max();
    if (a == inf)
        return "+\\infty";

    if (a == -inf)
        return "-\\infty";*/

    return boost::lexical_cast<std::string>(a);
}


PosetItem IntervalLattice::FromString( char const* begin, char const* end ) const
{
    if (streq(begin, end, "\\emptyset"))
        return GetUniversalMinimal();
    
    auto begin1 = std::find_if(begin, end, 
        [](char c){return c == '['||c=='(';});
    if (begin1 != end)
    {
        ++begin1;
        auto end1 = std::find(begin1, end, ':');
        if (end1 != end)
        {
            auto begin2 = end1+1;
            auto end2 = std::find_if(begin2, end, 
                [](char c){return c == ']'||c==')';});
            if (end2 != end)
                return PosetItem(
                    BoundFromString(begin1, end1), 
                    BoundFromString(begin2, end2));
        }
    }

    BOOST_THROW_EXCEPTION(error(""));
}


PosetItem::IntervalBound IntervalLattice::BoundFromString( char const* begin, char const* end ) const
{
    /*const auto inf = std::numeric_limits<PosetItem::IntervalBound>::max();
    if (streq(begin, end, "+\\infty"))
        return inf;

    if (streq(begin, end, "-\\infty"))
        return -inf;*/
    
    return boost::lexical_cast<PosetItem::IntervalBound>(begin, end - begin);
}




void IntervalLattice::BuildMaximalIndependent(
    PosetItem current, 
    PosetItems::const_iterator begin,
    PosetItems::const_iterator end,
    PosetItems& result) const
{
    while(begin != end && ! Preceq(*begin, current))
        ++begin;

    if (begin == end)
    {
        result.push_back(current);
        return;
    }    
    
    //const auto inf = std::numeric_limits<PosetItem::IntervalBound>::max();
    //if (begin->value.interval.max != inf)
    {
        PosetItem a(current.value.interval.min, begin->value.interval.max - 1);
        if (IsItem(a) && ! Preceq(*begin, a))
            BuildMaximalIndependent(a, begin+1, end, result);
    }

    //if (begin->value.interval.min != -inf)
    {
        PosetItem a(begin->value.interval.min + 1, current.value.interval.max);
        if (IsItem(a) && ! Preceq(*begin, a))
            BuildMaximalIndependent(a, begin+1, end, result);
    }
    
}


void IntervalLattice::GetMaximalIndependent( PosetItems const& A, PosetItems& maximal) const
{
    maximal.clear();
    //BuildMaximalIndependent(GetUniversalMaximal(), A.begin(), A.end(), maximal);
    
    for (auto& a : A)
    {
        if (! IsItem(a))
            continue;

        auto alen = GetLength(a);

        if (alen == 0)
            return maximal.clear();

        PosetItem b(_universalMaximal.value.interval.min, a.value.interval.max-1);
        for (auto& c: A)
        {
            if (! IsItem(b) || alen > GetLength(b)+1)
                break;
            if (IsItem(c) && Preceq(c, b))
                b.value.interval.min = c.value.interval.min + 1;
        }

        if (IsItem(b) && alen <= GetLength(b)+1)
            maximal.push_back(b);

        PosetItem d(a.value.interval.min+1, _universalMaximal.value.interval.max);
        for (auto& c: A)
        {
            if (! IsItem(d) || alen > GetLength(d)+1)
                break;
            if (IsItem(c) && Preceq(c, d))
                d.value.interval.max = c.value.interval.max - 1;
        }

        if (IsItem(d) && alen <= GetLength(d)+1)
            maximal.push_back(d);
    }
    
    if (maximal.empty())
        return;
    
    SelectUnique(maximal);
    //SelectIdealIndependent(A, maximal);
}


void IntervalLattice::GetMinimalIndependent(PosetItems const& B, PosetItems& minimal ) const
{
    if (B.empty())
        return GetMinimal(minimal);

    minimal.clear();

    for (auto& b: B)
    {
        if (IsMaximal(b))
            return;
    }
        
    for (int i = _universalMaximal.value.interval.min; i < 
        _universalMaximal.value.interval.max; )
    {
        bool changed = false;
        for (auto& a: B)
        {
            if (a.value.interval.min <= i &&
                i+1 <= a.value.interval.max)
            {
                i = a.value.interval.max;
                changed = true;
                if (i >= _universalMaximal.value.interval.max)
                    break;
            }
        }

        if (changed)
            continue;
        
        minimal.emplace_back(i,i+1);
        ++i;
    }

    //PosetItems result;
    for (auto&b:B)
    {
        if (IsMinimal(b))
            continue;        
        
        PosetItem left(b.value.interval.min-1, b.value.interval.min);
        PosetItem right(b.value.interval.max, b.value.interval.max+1);
        
        if (IsItem(left) && ! InIdeal(left, minimal))
            BuildMinimalIndependent(left, B.begin(), B.end(), minimal);

        if (IsItem(right) && ! InIdeal(right, minimal))
            BuildMinimalIndependent(right, B.begin(), B.end(), minimal);
    }
    
    SelectUnique(minimal);
    SelectMinimal(minimal);
}

void IntervalLattice::BuildMinimalIndependent( PosetItem current, PosetItems::const_iterator begin, PosetItems::const_iterator end, PosetItems& result ) const
{
    while(begin != end && ! Preceq(current, *begin))
        ++begin;
    
    if (begin == end)
    {
        result.push_back(current);
        return;
    }
        
    if (begin->value.interval.max != _universalMaximal.value.interval.max)
    {
        PosetItem a(current.value.interval.min, begin->value.interval.max + 1);
        if (IsItem(a) && Preceq(current, a) && ! Preceq(a, *begin) && ! InIdeal(a, result))
            BuildMinimalIndependent(a, begin+1, end, result);
    }

    if (begin->value.interval.min != _universalMaximal.value.interval.min)
    {
        PosetItem a(begin->value.interval.min - 1, current.value.interval.max);
        if (IsItem(a) && Preceq(current, a) && ! Preceq(a, *begin) && ! InIdeal(a, result))
            BuildMinimalIndependent(a, begin+1, end, result);
    }

}

bool IntervalLattice::PrecImmediate( 
    PosetItem const& a, PosetItem const& b ) const
{
    if  (! Prec(a, b) || ! IsItem(b))
        return false;
    auto la = GetLength(a);
    auto lb = GetLength(b);
    return la >=0 && lb == la+1;
}

void IntervalLattice::GetImmediatePrec( PosetItem const& item, PosetItems& items ) const
{
    items.clear();

    if (! IsItem(item) || IsMinimal(item))
        return;
    
    auto min = item.value.interval.min;
    auto max = item.value.interval.max;    
    if (min == max - 1)
        return items.emplace_back(0, 0);
    items.emplace_back(min+1, max);
    items.emplace_back(min, max-1);
}

void IntervalLattice::GetImmediateSucc( PosetItem const& item, PosetItems& items ) const
{
    items.clear();

    if (! IsItem(item))
        return;

    if (IsMinimal(item))
    {
        items.reserve(GetLength(_universalMaximal));
        for (int i = _universalMaximal.value.interval.min; i < _universalMaximal.value.interval.max; ++i)
            items.emplace_back(i, i+1);
        return;
    }

    auto min = item.value.interval.min;
    auto max = item.value.interval.max;
    
    if (min != _universalMaximal.value.interval.min)
        items.emplace_back(min-1, max);    
    if (max != _universalMaximal.value.interval.max)
        items.emplace_back(min, max+1);
}

void IntervalLattice::SelectUnique( PosetItems& items ) const
{
    for (auto& a: items)
        if (GetLength(a) == 0)
            a = GetUniversalMinimal();

    unique(items);
}

bool IntervalLattice::IsMaximalIndependent( PosetItem const& a, PosetItems const& A ) const
{
    if (A.empty())
        return IsMaximal(a);
    
    if (IsMinimal(a))
    {
        bool r = false;        
        for(auto& b:A)
        {
            if (GetLength(b) == 0)
                return false;
            r |= GetLength(b)==1;
        }        
        return r;
    }

    return PosetBase::IsMaximalIndependent(a, A);
}

void IntervalLattice::SetOptions(Options const& options)
{
    const auto inf = std::numeric_limits<PosetItem::IntervalBound>::max();
    _universalMaximal.value.interval.min = options.get("Min", -inf);
    _universalMaximal.value.interval.max = options.get("Max", inf);        
}

void IntervalLattice::SetDefaultOptions()
{
    const auto inf = std::numeric_limits<PosetItem::IntervalBound>::max();
    _universalMaximal.value.interval.min = -inf;
    _universalMaximal.value.interval.max = inf;
}

void IntervalLattice::GetMinUpperBounds( PosetItems const& items, PosetItems& upperBounds ) const
{
    PosetItem bound(0,0);
    for (auto a: items)
        bound = Span(a, bound);
    upperBounds.assign(1, bound);
}

void IntervalLattice::GetMaxLowerBounds( PosetItems const& items, PosetItems& lowerBounds ) const
{
    PosetItem bound = GetUniversalMaximal();
    for (auto a: items)
    {
        bound = Join(a, bound);
        if (GetLength(bound) == 0)
            break;
    }
    lowerBounds.assign(1, bound);
}

PosetItem IntervalLattice::Span( PosetItem const& i1, PosetItem const& i2 ) const
{
    if (GetLength(i1) == 0)
        return i2;

    if (GetLength(i2) == 0)
        return i1;

    return PosetItem(
        std::min(i1.value.interval.min, i2.value.interval.min), 
        std::max(i1.value.interval.max, i2.value.interval.max));
}

PosetItem IntervalLattice::Join( PosetItem const& i1, PosetItem const& i2 ) const
{
    if (Preceq(i1, i2))
        return i1;

    if (Preceq(i2, i1))
        return i2;

    if (i2.value.interval.min <= i1.value.interval.min && 
        i1.value.interval.min < i2.value.interval.max)
        return PosetItem(i1.value.interval.min, i2.value.interval.max);

    if (i1.value.interval.min <= i2.value.interval.min && 
        i2.value.interval.min < i1.value.interval.max)
        return PosetItem(i2.value.interval.min, i1.value.interval.max);

    return PosetItem(0,0);
}