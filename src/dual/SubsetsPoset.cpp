#include "stdafx.h"

#include "SubsetsPoset.h"




void SubsetsPoset::Register()
{
    Registrator<IPosetPtr>::Instance().RegisterFactory
        ("SubsetsPoset", [](){return std::make_shared<SubsetsPoset>();});
}

void SubsetsPoset::SetDefaultOptions()
{
    SetSize(sizeof(PosetItem::Subset)*8);
}

void SubsetsPoset::SetOptions(Options const& options)
{
    SetSize(options.get("Size", sizeof(PosetItem::Subset)*8));
}

void SubsetsPoset::GetImmediateSucc(PosetItem const& item, PosetItems& items) const
{
    items.clear();
    for (int j = 0; j != _size; ++j)
    {
        auto succ = item.value.subset | (1 << j);
        if (succ != item.value.subset)
            items.emplace_back(succ);
    }
}

void SubsetsPoset::GetImmediatePrec(PosetItem const& item, PosetItems& items) const
{
    items.clear();
    for (auto lb = find_first(item.value.subset); lb; ++lb)
    {
        int j = lb;
        items.emplace_back(item.value.subset ^ (1 << j));
    }
}

bool SubsetsPoset::PrecImmediate(PosetItem const& a, PosetItem const& b) const
{
    return Prec(a, b) && bits_count(a.value.subset ^ b.value.subset) == 1;
}

PosetItem SubsetsPoset::FromString(char const* begin, char const* end) const
{
    PosetItem::Subset subset;

    for (auto i = begin; i != end; ++i)
    {
        if (*i == '|' || i != begin)
        {
            auto j = boost::lexical_cast<int>(begin, i - begin);
            if (j < 0 || j >= _size)
                BOOST_THROW_EXCEPTION(std::out_of_range("Item number is out of range"));
            subset |= 1 << j;
            begin = i+1;
        }
    }

    if (begin != end)
    {
        auto j = boost::lexical_cast<int>(begin, end - begin);
        if (j < 0 || j >= _size)
            BOOST_THROW_EXCEPTION(std::out_of_range("Item number is out of range"));
        subset |= 1 << j;
    }

    return subset;
}

std::string SubsetsPoset::ToString(PosetItem const& item) const
{
    std::stringstream str;

    for (auto lb = find_first(item.value.subset); lb;)
    {
        int j = lb;
        ++lb;
        str << j;
        if (lb)
            str << '|';
    }

    return str.str();
}

PosetItem SubsetsPoset::GetUniversalMinimal() const
{
    return 0;
}

PosetItem SubsetsPoset::GetUniversalMaximal() const
{
    return _universalMaximal;
}

void SubsetsPoset::GetAllItems(PosetItems& items) const
{
    PosetItem::Subset count = 1;
    count <<= _size;
    if (count == 0)
        count = std::numeric_limits<PosetItem::Subset>::max();

    items.reserve(count);

    for (PosetItem::Subset s = 0; s != count; ++s)
        items.push_back(s);
}

bool SubsetsPoset::IsItem(PosetItem const& a) const
{
    return Preceq(a, _universalMaximal);
}

bool SubsetsPoset::Preceq(PosetItem const& left, PosetItem const& right) const
{
    return (left.value.subset & right.value.subset) == left.value.subset;
}

void SubsetsPoset::GetIndependentIntervals(PosetItems const& A, PosetItems const& B, PosetIntervals& independent) const
{
    throw std::logic_error("The method or operation is not implemented.");
}

void SubsetsPoset::GetMinimalIndependent(PosetItems const& A, PosetItems& minimal) const
{
    _idealRows.clear();
    _idealRows.reserve(A.size());

    for (auto& a : A)
        _idealRows.push_back(_universalMaximal.value.subset & ~a.value.subset);

    _dualizer.Dualize(_idealRows);

    minimal.clear();
    minimal.reserve(_dualizer.idealIndep.size());

    for (auto& a : _dualizer.idealIndep)
        minimal.push_back(a);
}

void SubsetsPoset::GetMaximalIndependent(PosetItems const& A, PosetItems& maximal) const
{
    _idealRows.clear();
    _idealRows.reserve(A.size());
    
    for (auto& a : A)
        _idealRows.push_back(a.value.subset);
    
    _dualizer.Dualize(_idealRows);

    maximal.clear();
    maximal.reserve(_dualizer.idealIndep.size());

    for (auto& a : _dualizer.idealIndep)
        maximal.push_back(_universalMaximal.value.subset & ~a);    
}

void SubsetsPoset::GetMinimal(PosetItems& minimal) const
{
    minimal.assign(1, GetUniversalMinimal());
}

void SubsetsPoset::GetMaximal(PosetItems& maximal) const
{
    maximal.assign(1, _universalMaximal);
}

void SubsetsPoset::SelectUnique(PosetItems& items) const
{
    unique(items);
}

bool SubsetsPoset::IsMinimalIndependent(PosetItem const& a, PosetItems const& B) const
{
    auto subset = a.value.subset;
    auto support = subset & 0;

    for (auto& b: B)
    {
        assert(IsItem(b));
        auto diff = subset & ~b.value.subset;
        auto lb = find_first(diff);
        if (! lb)
            return false;
        int j = lb;
        ++lb;
        if (! lb)
            support |= 1 << j;
    }

    return support == subset;
}

bool SubsetsPoset::IsMaximalIndependent(PosetItem const& a, PosetItems const& A) const
{
    auto inv = _universalMaximal.value.subset & ~a.value.subset;
    auto support = inv & 0;
    
    for (auto& b: A)
    {
        assert(IsItem(b));
        auto common = inv & b.value.subset;
        auto lb = find_first(common);
        if (! lb)
            return false;
        int j = lb;
        ++lb;
        if (! lb)
            support |= 1 << j;
    }

    return support == inv;
}

bool SubsetsPoset::IsMinimal(PosetItem const& a) const
{
    return IsUniversalMinimal(a);
}

bool SubsetsPoset::IsMaximal(PosetItem const& a) const
{
    return IsUniversalMaximal(a);
}

bool SubsetsPoset::Prec(PosetItem const& left, PosetItem const& right) const
{
    return left.value.subset != right.value.subset && 
        ((left.value.subset & right.value.subset) == left.value.subset);
}

bool SubsetsPoset::Equal(PosetItem const& left, PosetItem const& right) const
{
    return left.value.subset == right.value.subset;
}

void SubsetsPoset::SetSize(int size)
{
    if (size <= 0 || size > sizeof(PosetItem::Subset)*8)
        BOOST_THROW_EXCEPTION(std::out_of_range("Size is out of range"));

    _size = size;
    _universalMaximal.value.subset = (((PosetItem::Subset)1) << _size) - 1;
}

SubsetsPoset::SubsetsPoset(int size /*= sizeof(PosetItem::Subset)*8*/)
{
    SetSize(size);
}

void SubsetsPoset::GetMinUpperBounds( PosetItems const& items, PosetItems& upperBounds ) const
{
    PosetItem bound(0);
    for (auto& a:items)
        bound.value.subset |= a.value.subset;
    upperBounds.assign(1, bound);
}

void SubsetsPoset::GetMaxLowerBounds( PosetItems const& items, PosetItems& lowerBounds ) const
{
    PosetItem bound = _universalMaximal;
    for (auto& a:items)
        bound.value.subset &= a.value.subset;
    lowerBounds.assign(1, bound);
}