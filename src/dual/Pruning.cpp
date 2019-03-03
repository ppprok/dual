#include "Pruning.h"


CoveredRowsWeighter::CoveredRowsWeighter()
{
    Reset();
}

void CoveredRowsWeighter::Reset()
{
    _uncoveredRows.clear();
    _targetRows.clear();
    _coveredRows.clear();
    _pruner.Reset();
}


void CoveredRowsWeighter::Call( DualizationNode& node )
{
    switch (node.type)
    {
    case NodeType::NewNode:         
        WeightNewNode(node);
        _pruner.PrunInnerNode(node);
        break;

    case NodeType::BacktrackNode:        
        WeightBacktrackNode(node);
        break;
    
    case NodeType::CoverNode:
        _pruner.PrunCoverNode(node);
    }   

}



void CoveredRowsWeighter::WeightNewNode( DualizationNode &node )
{
    bit_chunk::get_bit_iterator j(node.lastColumn);
    
    for(int i = 0; i < (int)_uncoveredRows.size(); ++i)
    {
        int r = _uncoveredRows[i];
        Weight w = _pruner.GetWeight(r);

        if (! _targetRows[r].get(j))
            continue;
        
        CoveredRow ur={r, node.lastColumn, w};
        _coveredRows.push_back(ur);

        node.weight += w;        
        _pruner.DecRestWeight(w);
        
        _uncoveredRows[i] = _uncoveredRows.back();
        _uncoveredRows.pop_back();
        --i;
    }
}

void CoveredRowsWeighter::WeightBacktrackNode( DualizationNode &node )
{
    while(! _coveredRows.empty())
    {
        auto r = _coveredRows.back();
        if (r.j != node.lastColumn)
            break;
        
        _coveredRows.pop_back();
        _uncoveredRows.push_back(r.i);

        node.weight -= r.w;
        _pruner.IncRestWeight(r.w);
    }
}

void CoveredRowsWeighter::SetOptions( Options options )
{
    Reset();

    int m = options.TargetRows.size();
    options.RowsWeights.resize(m, options.DefaultWeight);
    _targetRows = std::move(options.TargetRows);
    _pruner.SetDefaultWeight(0);
    _pruner.SetWeights(std::move(options.RowsWeights));

    _uncoveredRows.reserve(m);
    for (int i = 0; i < m; ++i)
        _uncoveredRows.push_back(i);
    
}

void CoveredRowsWeighter::FixMaxBound( Weight bound )
{
    _pruner.FixMaxBound(bound);
}

void CoveredRowsWeighter::FixMinBound( Weight bound )
{
    _pruner.FixMinBound(bound);
}

void CoveredRowsWeighter::TargetToMin()
{
    _pruner.TargetToMin();
}

void CoveredRowsWeighter::TargetToMax()
{
    _pruner.TargetToMax();
}

void CoveredRowsWeighter::SetEpsilon( Weight eps )
{
    _pruner.SetEpsilon(eps);
}