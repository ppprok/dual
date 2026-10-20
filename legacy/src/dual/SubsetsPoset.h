#pragma once
#include "Configurable.h"
#include "PosetBase.h"
#include "Registrator.h"
#include "RuncFixed.h"
#include "bits.h"

/*!
    Решетка подмножеств
*/
class SubsetsPoset : public PosetBase, public IConfigurable {
    int _size;

    PosetItem _universalMaximal;

    typedef RuncFixed<PosetItem::Subset> Dualizer;

    mutable Dualizer _dualizer;

    mutable Dualizer::Rows _idealRows;

    mutable Dualizer::Rows _filterRows;

public:
    SubsetsPoset(int size = sizeof(PosetItem::Subset) * 8);

    void SetSize(int size);

    virtual bool Equal(PosetItem const& left, PosetItem const& right) const;

    virtual bool Prec(PosetItem const& left, PosetItem const& right) const;

    virtual bool IsMaximal(PosetItem const& a) const;

    virtual bool IsMinimal(PosetItem const& a) const;

    virtual bool IsMaximalIndependent(PosetItem const& a, PosetItems const& A) const;

    virtual bool IsMinimalIndependent(PosetItem const& a, PosetItems const& B) const;

    virtual void SelectUnique(PosetItems& items) const;

    virtual void GetMaximal(PosetItems& maximal) const;

    virtual void GetMinimal(PosetItems& minimal) const;

    virtual void GetMaximalIndependent(PosetItems const& A, PosetItems& maximal) const;

    virtual void GetMinimalIndependent(PosetItems const& A, PosetItems& minimal) const;

    virtual void GetIndependentIntervals(PosetItems const& A, PosetItems const& B, PosetIntervals& independent) const;

    virtual bool Preceq(PosetItem const& left, PosetItem const& right) const;

    virtual bool IsItem(PosetItem const& a) const;

    virtual void GetAllItems(PosetItems& items) const;

    virtual PosetItem GetUniversalMaximal() const;

    virtual PosetItem GetUniversalMinimal() const;

    virtual std::string ToString(PosetItem const& item) const;

    virtual PosetItem FromString(char const* begin, char const* end) const;

    virtual bool PrecImmediate(PosetItem const& a, PosetItem const& b) const;

    virtual void GetImmediatePrec(PosetItem const& item, PosetItems& items) const;

    virtual void GetImmediateSucc(PosetItem const& item, PosetItems& items) const;

    virtual void SetOptions(Options const& options);

    virtual void SetDefaultOptions();

    static void Register();

    virtual void GetMinUpperBounds(PosetItems const& items, PosetItems& upperBounds) const;

    virtual void GetMaxLowerBounds(PosetItems const& items, PosetItems& lowerBounds) const;
};