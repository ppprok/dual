#pragma once

#include "CSVLoader.h"
#include "PosetItem.h"
#include <memory>
#include <vector>

// type of item
//typedef int ItemId;

//typedef __int64 Number;

struct too_many_items_error : std::runtime_error {};

//////////////////////////////////////////////////////////////////////////
// Partialy ordered set (poset) interface
//////////////////////////////////////////////////////////////////////////
struct IPoset {

    // Check relation left <= right
    virtual bool Preceq(PosetItem const& left, PosetItem const& right) const = 0;

    // Check relation left < right
    virtual bool Prec(PosetItem const& left, PosetItem const& right) const = 0;

    // Check relation left = right
    virtual bool Equal(PosetItem const& left, PosetItem const& right) const = 0;

    // Is "a" maximal item from A
    virtual bool IsMaximal(PosetItem const& a, PosetItems const& A) const = 0;

    // Is "a" maximal item from A
    virtual bool IsMinimal(PosetItem const& a, PosetItems const& A) const = 0;

    // Is "a" belong to poset
    virtual bool IsItem(PosetItem const& a) const = 0;

    // Is "a" maximal item
    virtual bool IsMaximal(PosetItem const& a) const = 0;

    // Is "a" minimal item
    virtual bool IsMinimal(PosetItem const& a) const = 0;

    // Is "a" maximal from independent for A
    virtual bool IsMaximalIndependent(PosetItem const& a, PosetItems const& A) const = 0;

    // Is "a" minimal from independent for B
    virtual bool IsMinimalIndependent(PosetItem const& a, PosetItems const& B) const = 0;

    // Not in ideal for A and not in filter for B
    virtual bool IsIndependent(PosetItem const& a, PosetItems const& A, PosetItems const& B) const = 0;

    // Any b in A : b <= a
    virtual bool InIdeal(PosetItem const& a, PosetItems const& A) const = 0;

    // Any b in A : a <= b
    virtual bool InFilter(PosetItem const& a, PosetItems const& A) const = 0;

    // Get maximal items from A
    virtual void SelectMaximal(PosetItems& A) const = 0;

    // Get minimal items from A
    virtual void SelectMinimal(PosetItems& A) const = 0;

    // Get independent for ideal A items from B
    virtual void SelectIdealIndependent(PosetItems const& A, PosetItems& B) const = 0;

    // Get independent for filter A items from B
    virtual void SelectFilterIndependent(PosetItems const& A, PosetItems& B) const = 0;

    // Get independent for ideal A and filter B items from X
    virtual void SelectIndependent(PosetItems const& A, PosetItems const& B, PosetItems& X) const = 0;

    // Get ideal of A intersected with B
    virtual void SelectIdeal(PosetItems const& A, PosetItems& B) const = 0;

    // Get filter of A intersected with B
    virtual void SelectFilter(PosetItems const& A, PosetItems& B) const = 0;

    // Delete duplicates by Equal relation
    virtual void SelectUnique(PosetItems& items) const = 0;

    // Select all items of poset
    virtual void GetAllItems(PosetItems& items) const = 0;

    // Get all maximal items
    virtual void GetMaximal(PosetItems& maximal) const = 0;

    // Get all minimal items
    virtual void GetMinimal(PosetItems& minimal) const = 0;

    // Get single maximal item
    virtual PosetItem GetUniversalMaximal() const = 0;

    // Get single minimal item
    virtual PosetItem GetUniversalMinimal() const = 0;

    // Get all maximal independent for ideal A items
    virtual void GetMaximalIndependent(PosetItems const& A, PosetItems& maximal) const = 0;

    // Get all minimal independent for filder A items
    virtual void GetMinimalIndependent(PosetItems const& A, PosetItems& minimal) const = 0;

    // Enumerate all intervals (x, y),
    // where x <= y,
    // y --- maximal independent for ideal A,
    // x --- minimal independent for filter B
    virtual void GetIndependentIntervals(PosetItems const& A,
                                         PosetItems const& B,
                                         PosetIntervals& independent) const = 0;

    // Get text presentation of item
    virtual std::string ToString(PosetItem const& item) const = 0;

    // Convert string to poset item
    virtual PosetItem FromString(char const* begin, char const* end) const = 0;

    // Order items topologicaly: (a_i<=a_j) => (i<=j)
    virtual void TopologicalSort(PosetItems& items) const = 0;

    // Check topological ordered
    virtual bool IsTopologicalOrdered(PosetItems const& items) const = 0;

    // a < b and not exist c such that a <= c <= b
    virtual bool PrecImmediate(PosetItem const& a, PosetItem const& b) const = 0;

    // Get immediate precessors, a: a < item, a --- maximal
    virtual void GetImmediatePrec(PosetItem const& item, PosetItems& items) const = 0;

    // Get immediate successors, a: a > item, a --- minimal
    virtual void GetImmediateSucc(PosetItem const& item, PosetItems& items) const = 0;

    virtual bool IsUniversalMaximal(PosetItem const& item) const = 0;

    virtual bool IsUniversalMinimal(PosetItem const& item) const = 0;

    /*!
        Перечислить минимальные верхние границы для набора элементов
    */
    virtual void GetMinUpperBounds(PosetItems const& items, PosetItems& upperBounds) const = 0;

    /*!
        Перечислить максимальные нижние границы для набора элементов
    */
    virtual void GetMaxLowerBounds(PosetItems const& items, PosetItems& lowerBounds) const = 0;

    // Empty destructor
    virtual ~IPoset(){};
};

typedef std::shared_ptr<IPoset> IPosetPtr;

typedef std::vector<IPosetPtr> Posets;

std::string ToString(IPosetPtr poset, PosetItems const& items, std::string const& separator = " ");

bool AllPreceq(IPoset const& poset, PosetItems const& left, PosetItem const& right);

bool Contains(IPoset const& poset, PosetItems const& collection, PosetItem const& item);
