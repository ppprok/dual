#pragma once
#include "IPoset.h"
#include "Configurable.h"

/*!
    Reversed poset:
        a <= b <---> b <= a
        min <---> max
*/
class ReversedPoset
    :public IPoset, public IConfigurable
{
    IPosetPtr _inner;

public:

    ReversedPoset(IPosetPtr const& poset);

    ReversedPoset();

    static void Register();

    IPosetPtr const& GetInnerPoset() const
    {
        return _inner;
    }

    IPosetPtr GetInnerPoset()
    {
        return _inner;
    }

    virtual bool Preceq( PosetItem const& left, PosetItem const& right ) const;

    virtual bool Prec( PosetItem const& left, PosetItem const& right ) const;

    virtual bool Equal( PosetItem const& left, PosetItem const& right ) const;

    virtual bool IsMaximal( PosetItem const& a, PosetItems const& A ) const;

    virtual bool IsMaximal( PosetItem const& a ) const;
    
    virtual bool IsMinimal( PosetItem const& a, PosetItems const& A ) const;

    virtual bool IsMinimal( PosetItem const& a ) const;
    
    virtual bool IsItem( PosetItem const& a ) const;

    virtual bool IsMaximalIndependent( PosetItem const& a, PosetItems const& A ) const;

    virtual bool IsMinimalIndependent( PosetItem const& a, PosetItems const& B ) const;

    virtual bool IsIndependent( PosetItem const& a, PosetItems const& A, PosetItems const& B ) const;

    virtual bool InIdeal( PosetItem const& a, PosetItems const& A ) const;

    virtual bool InFilter( PosetItem const& a, PosetItems const& A ) const;

    virtual void SelectMaximal( PosetItems& A ) const;

    virtual void SelectMinimal( PosetItems& A ) const;

    virtual void SelectIdealIndependent( PosetItems const& A, PosetItems& B ) const;

    virtual void SelectFilterIndependent( PosetItems const& A, PosetItems& B ) const;

    virtual void SelectIndependent( PosetItems const& A, PosetItems const& B, PosetItems& X ) const;

    virtual void SelectIdeal( PosetItems const& A, PosetItems& B ) const;

    virtual void SelectFilter( PosetItems const& A, PosetItems& B ) const;

    virtual void SelectUnique( PosetItems& items ) const;

    virtual void GetAllItems( PosetItems& items ) const;

    virtual void GetMaximal( PosetItems& maximal ) const;

    virtual void GetMinimal( PosetItems& minimal ) const;

    virtual PosetItem GetUniversalMaximal() const;

    virtual PosetItem GetUniversalMinimal() const;

    virtual void GetMaximalIndependent( PosetItems const& A, PosetItems& maximal ) const;

    virtual void GetMinimalIndependent( PosetItems const& A, PosetItems& minimal ) const;

    virtual void GetIndependentIntervals( PosetItems const& A, PosetItems const& B, PosetIntervals& independent ) const;

    virtual std::string ToString( PosetItem const& item ) const;

    virtual PosetItem FromString( char const* begin, char const* end ) const;

    virtual void TopologicalSort( PosetItems& items ) const;

    virtual bool IsTopologicalOrdered( PosetItems const& items ) const;

    virtual bool PrecImmediate( PosetItem const& a, PosetItem const& b ) const;

    virtual void GetImmediatePrec( PosetItem const& item, PosetItems& items ) const;

    virtual void GetImmediateSucc( PosetItem const& item, PosetItems& items ) const;

    virtual bool IsUniversalMaximal( PosetItem const& item ) const;

    virtual bool IsUniversalMinimal( PosetItem const& item ) const;

    virtual void SetOptions( Options const& options );

    virtual void SetDefaultOptions();

    virtual void GetMinUpperBounds( PosetItems const& items, PosetItems& upperBounds ) const;

    virtual void GetMaxLowerBounds( PosetItems const& items, PosetItems& lowerBounds ) const;
};

IPosetPtr Reverse(IPosetPtr const& poset);