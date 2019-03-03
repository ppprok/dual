
#include "GraphPoset.h"
#include "graph_algorithms.h"
#include "Registrator.h"


void GraphPoset::Register()
{
    Registrator<IPosetPtr>::Instance().RegisterFactory
        ("GraphPoset", [](){return std::make_shared<GraphPoset>();});
}

void GraphPoset::SetDefaultOptions()
{
    SetGraph(0, DirectedEdges());
}

void GraphPoset::SetOptions( Options const& options )
{
    int n = options.get("Size", 0);
    
    DirectedEdges edges;
    for(auto& b : options.get_child("DirectedEdges", Options()))
    {
        auto v1 = boost::lexical_cast<int>(b.first.empty() ? b.second.data() : b.first);
        if (n <= v1)
            n = v1+1;
        
        for (auto& a : b.second)
        {
            auto v2 = boost::lexical_cast<int>(a.first.empty() ? a.second.data() : a.first);
            if (n <= v2)
                n = v2+1;
            
            edges.emplace_back( v1, v2);
        }
    }
        
    SetGraph(n, edges);
}

bool GraphPoset::IsMaximal( PosetItem const& a ) const
{
    return IsItem(a) && _maximal.get(a.value.vertex);
}

bool GraphPoset::IsMinimal( PosetItem const& a ) const
{
    return IsItem(a) && _minimal.get(a.value.vertex);
}


bool GraphPoset::Preceq( PosetItem const& left, PosetItem const& right ) const
{
    if (left == right)
        return true;

    if (IsUniversalMaximal(right) || IsUniversalMinimal(left))
        return true;

    if (IsUniversalMinimal(right) || IsUniversalMaximal(left))
        return false;

    if (! IsItem(left) || ! IsItem(right))
        return false;

    return _adjacencyMatrix.get(left.value.vertex, right.value.vertex);
}


void GraphPoset::SetGraph(int n, DirectedEdges const& edges )
{
    if (n < 0)
        BOOST_THROW_EXCEPTION(std::out_of_range("Size of graph must be not negative"));
       
    _size = n;

    create_adjacency_matrix(n, edges, _adjacencyMatrix);
    topological_order(_adjacencyMatrix, _topoOrder);
    
    redirect_graph(_adjacencyMatrix, _adjacencyMatrixT);
    redirect_graph(_adjacencyMatrix, _precessors);

    CollectMinimalAndMaximalItems();
            
    transitive_closure(_adjacencyMatrix);
    transitive_closure(_adjacencyMatrixT);
    
    CorrectPrecessors();    
    redirect_graph(_precessors, _successors);
}

void GraphPoset::TopologicalSort( PosetItems& items ) const
{
    std::sort(items.begin(), items.end(),
        [this](PosetItem const& a, PosetItem const& b){return TopoLess(a, b);});
}

void GraphPoset::CorrectPrecessors()
{
    bit_vector temp(_size);
    for (int i = 0; i != _size; ++i)
    {
        auto& precessors = _precessors.get_row(i);
        for (int j = precessors.find_first(); j != -1; j = precessors.find_next(j))
        {
            temp.assign_and(_adjacencyMatrix.get_row(j), _adjacencyMatrixT.get_row(i));
            temp.reset(i);
            temp.reset(j);

            if(temp.any())
                _precessors.set(i, j, false);
        }
    }
}


void GraphPoset::GetMaximal( PosetItems& items ) const
{
    items = _maximalItems;
}

void GraphPoset::GetMinimal( PosetItems& items ) const
{
    items = _minimalItems;
}

bool GraphPoset::PrecImmediate( PosetItem const& a, PosetItem const& b ) const
{
    if (IsUniversalMinimal(a) && IsUniversalMaximal(b))
        return _size == 0;
    
    if (IsUniversalMinimal(a))
        return IsMinimal(b);
    
    if (IsUniversalMaximal(b))
        return IsMaximal(a);
            
    return IsItem(a) && IsItem(b) && _precessors.get(b.value.vertex, a.value.vertex);
}

void GraphPoset::GetImmediatePrec( PosetItem const& item, PosetItems& items ) const
{
    if (! IsItem(item))
    {
        if (IsUniversalMaximal(item))
            return GetMaximal(items);
        else
            return items.clear();
    }
    
    items.clear();
    _precessors.get_row(item.value.vertex).get_bits(items);
}

void GraphPoset::GetImmediateSucc( PosetItem const& item, PosetItems& items ) const
{
    if (! IsItem(item))
    {
        if (IsUniversalMinimal(item))
            return GetMinimal(items);
        else
            return items.clear();
    }

    items.clear();
    _successors.get_row(item.value.vertex).get_bits(items);    
}


void GraphPoset::SelectUnique( PosetItems& items ) const
{
    unique(items);
}

bool GraphPoset::TopoLess( PosetItem const& a, PosetItem const& b ) const
{
    int av = a.value.vertex, bv = b.value.vertex;

    if (av == bv)
        return false;

    if (inside(_topoOrder, av))
        av = _topoOrder[av];

    if (inside(_topoOrder, bv))
        bv = _topoOrder[bv];

    return av > bv;
}

bool GraphPoset::IsTopologicalOrdered( PosetItems const& items ) const
{
    return std::is_sorted(items.begin(), items.end(),
        [this](PosetItem const& a, PosetItem const& b){return TopoLess(a, b);});
}

bool GraphPoset::IsItem( PosetItem const& a ) const
{
    return a.value.vertex >= 0 && a.value.vertex < _size;
}

void GraphPoset::GetAllItems( PosetItems& items ) const
{
    items.clear();
    items.reserve(_size);
    for (int i = 0; i != _size; ++i)
        items.emplace_back(i);
}

PosetItem GraphPoset::GetUniversalMaximal() const
{
    if (_maximalItems.size()==1)
        return _maximalItems.front();
    
    return _size;
}

PosetItem GraphPoset::GetUniversalMinimal() const
{
    if (_minimalItems.size()==1)
        return _minimalItems.front();
    
    return -1;
}

std::string GraphPoset::ToString( PosetItem const& item ) const
{
    if (IsItem(item))
        return boost::lexical_cast<std::string>(item.value.vertex);
    
    if (IsUniversalMaximal(item))
        return "+\\infty";

    if (IsUniversalMinimal(item))
        return "-\\infty";

    return boost::lexical_cast<std::string>(item.value.vertex);
}

PosetItem GraphPoset::FromString( char const* begin, char const* end ) const
{
    if (streq(begin, end,"+\\infty"))
        return GetUniversalMaximal();

    if (streq(begin, end,"-\\infty"))
        return GetUniversalMinimal();

    return boost::lexical_cast<int>(begin, end - begin);
}

int GraphPoset::GetSize() const
{
    return _size;
}

GraphPoset::GraphPoset()
{
    SetDefaultOptions();
}

void GraphPoset::CollectMinimalAndMaximalItems()
{
    _maximal.zero(_size);
    _maximalItems.clear();
    _minimal.zero(_size);
    _minimalItems.clear();

    for (int i = 0; i != _size; ++i)
    {
        _adjacencyMatrix.set(i, i, false);
        _adjacencyMatrixT.set(i, i, false);

        if (_adjacencyMatrix.get_row(i).none())
        {
            _maximal.set(i);
            _maximalItems.push_back(i);
        }

        if (_adjacencyMatrixT.get_row(i).none())
        {
            _minimal.set(i);
            _minimalItems.push_back(i);
        }
    }
}

bool GraphPoset::Equal( PosetItem const& left, PosetItem const& right ) const
{
    return left.value.vertex == right.value.vertex;
}

void GraphPoset::GetMinUpperBounds( PosetItems const& items, PosetItems& upperBounds ) const
{
    upperBounds.clear();
    if (items.empty() || ! IsItem(items[0]))
        return;

    _temp.zero(_size);
    bit_chunk bits = _temp;
    bits.assign(_adjacencyMatrix.get_row(items[0].value.vertex));
    bits.set(items[0].value.vertex);

    for (int i = 1, n = items.size(); i !=n; ++i)
    {
        if (! IsItem(items[i]))
            return;
        
        auto j = items[i].value.vertex;
        auto r = bits.get(j);
        bits.assign_and(bits, _adjacencyMatrix.get_row(j));
        if (r)
            bits.set(j);
    }

    for (auto i = bits.find_first(); i != -1; i = bits.find_next(i))
        upperBounds.emplace_back(i);
    
    SelectMinimal(upperBounds);
}

void GraphPoset::GetMaxLowerBounds( PosetItems const& items, PosetItems& lowerBounds ) const
{
    lowerBounds.clear();
    if (items.empty() || ! IsItem(items[0]))
        return;

    _temp.zero(_size);
    bit_chunk bits = _temp;
    bits.assign(_adjacencyMatrixT.get_row(items[0].value.vertex));
    bits.set(items[0].value.vertex);

    for (int i = 1, n = items.size(); i !=n; ++i)
    {
        if (! IsItem(items[i]))
            return;

        auto j = items[i].value.vertex;
        auto r = bits.get(j);
        bits.assign_and(bits, _adjacencyMatrixT.get_row(j));
        if (r)
            bits.set(j);
    }

    for (auto i = bits.find_first(); i != -1; i = bits.find_next(i))
        lowerBounds.emplace_back(i);

    SelectMaximal(lowerBounds);
}