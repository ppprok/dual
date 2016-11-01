#pragma once

#include <limits>

#include "PosetBase.h"


// Type of item name
typedef std::string ItemName;

typedef std::vector<ItemName> ItemNames;


//////////////////////////////////////////////////////////////////////////
// Antichain is a poset in which a<=b iff a==b
//////////////////////////////////////////////////////////////////////////
class Antichain
    :public PosetBase
{
protected:

    bool _includeUniversalMinimal;

    bool _includeUniversalMaximal;

    int _size;
    
public:

    Antichain();

    Antichain(int size, bool umin = false, bool umax = false)
        :_size(size), _includeUniversalMinimal(umin), _includeUniversalMaximal(umax)
    {
    }

    int GetSize() const
    {
        return _size;
    }

    void SetSize(int size)
    {
        _size = size;
    }
        
    bool IsUniversalMaximal(PosetItem const& item) const;

    bool IsUniversalMinimal(PosetItem const& item) const;

    virtual void SetOptions(Options const& options);

    virtual void SetDefaultOptions();

    static void Register();

    virtual bool Equal( PosetItem const& left, PosetItem const& right ) const;

    virtual bool Prec( PosetItem const& left, PosetItem const& right ) const;
        
    virtual bool IsMaximal( PosetItem const& a ) const;

    virtual bool IsMinimal( PosetItem const& a ) const;
        
    virtual void SelectUnique( PosetItems& items ) const;

    virtual void TopologicalSort( PosetItems& items ) const;
    
    virtual bool Preceq( PosetItem const& left, PosetItem const& right ) const;

    virtual bool IsItem( PosetItem const& a ) const;

    virtual bool IsMaximalIndependent( PosetItem const& a, PosetItems const& A ) const;

    virtual bool IsMinimalIndependent( PosetItem const& a, PosetItems const& B ) const;

    virtual void GetMaximal( PosetItems& maximal ) const;

    virtual void GetMinimal( PosetItems& minimal ) const;
        
    virtual std::string ToString( PosetItem const& item ) const;

    virtual PosetItem FromString( char const* begin, char const* end ) const;

    virtual void GetAllItems( PosetItems& items ) const;

    virtual PosetItem GetUniversalMaximal() const;

    virtual PosetItem GetUniversalMinimal() const;

    virtual bool PrecImmediate( PosetItem const& a, PosetItem const& b ) const;

    virtual void GetImmediatePrec( PosetItem const& item, PosetItems& items ) const;

    virtual void GetImmediateSucc( PosetItem const& item, PosetItems& items ) const;
};