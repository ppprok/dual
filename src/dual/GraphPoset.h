#pragma once

#include "bit_matrix.h"
#include "PosetBase.h"
//#include "Antichain.h"
#include "bit_vector.h"

// Edge in graph: if a->b then a<=b
typedef std::pair<int, int> DirectedEdge;

typedef std::vector<DirectedEdge> DirectedEdges;

//////////////////////////////////////////////////////////////////////////
// Poset defined by directed graph
//////////////////////////////////////////////////////////////////////////
class GraphPoset
    :public PosetBase, public IConfigurable
{
private:

    // Number of verticies
    int _size;

    // Poset graph adjacency boolean matrix
    bit_matrix _adjacencyMatrix;

    // Poset graph transposed adjacency boolean matrix
    bit_matrix _adjacencyMatrixT;

    // Precessors of items
    bit_matrix _precessors;
        
    // Successors of items
    bit_matrix _successors;

    // Maximal items mask
    bit_vector _maximal;

    PosetItems _maximalItems;

    // Minimal items mask
    bit_vector _minimal;

    PosetItems _minimalItems;

    // Tolopogical order of verticies
    std::vector<int> _topoOrder;   

    mutable bit_vector _temp;

private:
  
    void CorrectPrecessors();

    bool TopoLess(PosetItem const& a, PosetItem const& b) const;
    
    void CollectMinimalAndMaximalItems();


public:
        
    // Setup graph edges
    void SetGraph(int size, DirectedEdges const& edges);
    
    int GetSize() const;
    
    virtual void SetOptions( Options const& options);

    virtual void SetDefaultOptions();

    static void Register();

    GraphPoset();

    virtual bool IsMaximal( PosetItem const& a ) const;
    
    virtual bool IsMinimal( PosetItem const& a ) const;
    
    virtual bool Preceq( PosetItem const& left, PosetItem const& right ) const;
            
    virtual bool PrecImmediate( PosetItem const& a, PosetItem const& b ) const;
    
    virtual void GetImmediatePrec( PosetItem const& item, PosetItems& items ) const;

    virtual void GetImmediateSucc( PosetItem const& item, PosetItems& items ) const;

    virtual void SelectUnique( PosetItems& items ) const;
        
    virtual void TopologicalSort( PosetItems& items ) const;

    virtual bool IsTopologicalOrdered( PosetItems const& items ) const;

    virtual bool IsItem( PosetItem const& a ) const;

    virtual void GetAllItems( PosetItems& items ) const;

    virtual PosetItem GetUniversalMaximal() const;

    virtual PosetItem GetUniversalMinimal() const;

    virtual std::string ToString( PosetItem const& item ) const;

    virtual PosetItem FromString( char const* begin, char const* end ) const;
    
    virtual void GetMaximal( PosetItems& items ) const;
    
    virtual void GetMinimal( PosetItems& items ) const;

    virtual bool Equal( PosetItem const& left, PosetItem const& right ) const;

    virtual void GetMinUpperBounds( PosetItems const& items, PosetItems& upperBounds ) const;

    virtual void GetMaxLowerBounds( PosetItems const& items, PosetItems& lowerBounds ) const;
};