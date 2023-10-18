#pragma once

#include "Configurable.h"
#include "IPoset.h"

//////////////////////////////////////////////////////////////////////////
// Base implementation of IPoset methods.
// PosetBase is abstract class
//////////////////////////////////////////////////////////////////////////
class PosetBase : public IPoset {

    mutable PosetItems _temp;

public:
    //////////////////////////////////////////////////////////////////////////
    // IPoset Implementation
    //////////////////////////////////////////////////////////////////////////

    virtual bool Equal(PosetItem const& left, PosetItem const& right) const;

    virtual bool Prec(PosetItem const& left, PosetItem const& right) const;

    virtual bool IsMaximal(PosetItem const& a, PosetItems const& A) const;

    virtual bool IsMaximal(PosetItem const& a) const;

    virtual bool IsMinimal(PosetItem const& a, PosetItems const& A) const;

    virtual bool IsMinimal(PosetItem const& a) const;

    virtual bool IsIndependent(PosetItem const& a, PosetItems const& A, PosetItems const& B) const;

    virtual bool InIdeal(PosetItem const& a, PosetItems const& A) const;

    virtual bool InFilter(PosetItem const& a, PosetItems const& A) const;

    virtual bool IsMaximalIndependent(PosetItem const& a, PosetItems const& A) const;

    virtual bool IsMinimalIndependent(PosetItem const& a, PosetItems const& B) const;

    virtual void SelectMaximal(PosetItems& A) const;

    virtual void SelectIndependent(PosetItems const& A, PosetItems const& B, PosetItems& X) const;

    virtual void SelectFilterIndependent(PosetItems const& A, PosetItems& B) const;

    virtual void SelectIdealIndependent(PosetItems const& A, PosetItems& B) const;

    virtual void SelectIdeal(PosetItems const& A, PosetItems& B) const;

    virtual void SelectFilter(PosetItems const& A, PosetItems& B) const;

    virtual void SelectUnique(PosetItems& items) const;

    virtual void SelectMinimal(PosetItems& A) const;

    virtual void GetMaximal(PosetItems& maximal) const;

    virtual void GetMinimal(PosetItems& minimal) const;

    virtual void GetMaximalIndependent(PosetItems const& A, PosetItems& maximal) const;

    virtual void GetMinimalIndependent(PosetItems const& A, PosetItems& minimal) const;

    virtual void GetIndependentIntervals(PosetItems const& A, PosetItems const& B, PosetIntervals& independent) const;

    virtual void TopologicalSort(PosetItems& items) const;

    virtual bool IsTopologicalOrdered(PosetItems const& items) const;

    PosetItem FromStdString(std::string const& s) const;

    virtual bool IsUniversalMaximal(PosetItem const& item) const;

    virtual bool IsUniversalMinimal(PosetItem const& item) const;

private:
    void DoTopologicalSort(PosetItems& items, int n, int i) const;
};