#pragma once

#include "NumberChain.h"

//////////////////////////////////////////////////////////////////////////
// Lattice of open and closed intervals [a, b], (c, d) contained by (min, max)
// Maximal: (min, max).
// Minimal: \emptyset.
// Order: (i1 <= i2) <=> i1 \subseteq i2.
//////////////////////////////////////////////////////////////////////////
class IntervalLattice : public PosetBase, public IConfigurable {

    PosetItem::IntervalBound BoundFromString(char const* begin, char const* end) const;

    std::string ToString(PosetItem::IntervalBound a) const;

    int GetLength(PosetItem const& a) const {
        return (int) a.value.interval.max - (int) a.value.interval.min;
    }

    PosetItem _universalMaximal;

public:
    IntervalLattice() {
        SetDefaultOptions();
    }

    IntervalLattice(PosetItem::IntervalBound min, PosetItem::IntervalBound max) : _universalMaximal(min, max) {}

    static void Register();

    virtual void SetOptions(Options const& options);

    virtual void SetDefaultOptions();

    virtual bool IsItem(PosetItem const& a) const;

    virtual bool IsMaximal(PosetItem const& a) const;

    virtual void SelectUnique(PosetItems& items) const;

    virtual std::string ToString(PosetItem const& item) const;

    virtual void GetMaximal(PosetItems& maximal) const;

    virtual bool Preceq(PosetItem const& left, PosetItem const& right) const;

    virtual void GetMaximalIndependent(PosetItems const& A, PosetItems& maximal) const;

    virtual bool Equal(PosetItem const& left, PosetItem const& right) const;

    virtual bool Prec(PosetItem const& left, PosetItem const& right) const;

    virtual void GetMinimal(PosetItems& minimal) const;

    virtual void GetMinimalIndependent(PosetItems const& B, PosetItems& minimal) const;

    virtual void GetAllItems(PosetItems& items) const;

    virtual bool IsMinimal(PosetItem const& a) const;

    virtual PosetItem GetUniversalMaximal() const;

    virtual PosetItem GetUniversalMinimal() const;

    virtual PosetItem FromString(char const* begin, char const* end) const;

    virtual bool PrecImmediate(PosetItem const& a, PosetItem const& b) const;

    virtual void GetImmediatePrec(PosetItem const& item, PosetItems& items) const;

    virtual void GetImmediateSucc(PosetItem const& item, PosetItems& items) const;

    virtual void GetMinUpperBounds(PosetItems const& items, PosetItems& upperBounds) const;

    virtual void GetMaxLowerBounds(PosetItems const& items, PosetItems& lowerBounds) const;

    /*!
        Наименьшая верхняя грань двух промежутков
    */
    PosetItem Span(PosetItem const& i1, PosetItem const& i2) const;

    /*!
        Наибольшая нижняя грань двух промежутков
    */
    PosetItem Join(PosetItem const& i1, PosetItem const& i2) const;

private:
    void BuildMaximalIndependent(PosetItem current,
                                 PosetItems::const_iterator begin,
                                 PosetItems::const_iterator end,
                                 PosetItems& result) const;

    void BuildMinimalIndependent(PosetItem current,
                                 PosetItems::const_iterator begin,
                                 PosetItems::const_iterator end,
                                 PosetItems& result) const;

    virtual bool IsMaximalIndependent(PosetItem const& a, PosetItems const& A) const;
};