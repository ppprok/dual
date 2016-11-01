#include "stdafx.h"

#include "Weighters.h"

CoveredRowWeighterCallback::CoveredRowWeighterCallback()
{
    Reset();
}


void CoveredRowWeighterCallback::Reset()
{
    WeighterBase::Reset();
    _coveredRows.clear();
    _targetRows.clear();
    _uncoveredRows.clear();
}


void CoveredRowWeighterCallback::SetTargetRows( bit_chunks targetRows )
{
    _targetRows = std::move(targetRows);

    int m = _targetRows.size();
    _uncoveredRows.resize(m);
    for (int i = 0; i < m; ++i)
        _uncoveredRows[i] = i;

    _coveredRows.clear();
    _coveredRows.reserve(m);

    _weightSumCount = m;

    InvalidateRestWeights();
}


void CoveredRowWeighterCallback::WeightNewNode( DualizationNode &node )
{
    bit_chunk::get_bit_iterator j(node.lastColumn);

    if ((int)node.columns.size() <= 1)
    {
        // Инициализация веса
        node.weight = 0;        
    }

    for(int i = 0; i < (int)_uncoveredRows.size(); ++i)
    {
        int r = _uncoveredRows[i];

        if (! _targetRows[r].get(j))
            continue;

        Weight w = GetWeight(r);
        node.weight += w;
        DecRestWeight(w);

        CoveredRow cr={r, node.lastColumn, w};
        _coveredRows.push_back(cr);
        
        _uncoveredRows[i] = _uncoveredRows.back();
        _uncoveredRows.pop_back();
        --i;
    }
}


void CoveredRowWeighterCallback::WeightBacktrackNode( DualizationNode &node )
{
    while(! _coveredRows.empty())
    {
        auto r = _coveredRows.back();
        if (r.j != node.lastColumn)
            break;

        _coveredRows.pop_back();
        _uncoveredRows.push_back(r.i);

        node.weight -= r.w;        
        IncRestWeight(r.w);
    }
}


void ColumnsWeighterCallback::WeightNewNode( DualizationNode &node )
{
    if (node.columns.size() <= 1)
        node.weight = 0;

    Weight w = GetWeight(node.lastColumn);
    node.weight += w;
    DecRestWeight(w);
}


void ColumnsWeighterCallback::WeightBacktrackNode( DualizationNode &node )
{
    Weight w = GetWeight(node.lastColumn);
    node.weight -= w;
    IncRestWeight(w);
}



bool WeighterBase::IsRestWeightsValid() const
{
    return _restPositive >= 0 && _restNegative <= 0;
}


void WeighterBase::InvalidateRestWeights()
{
    _restNegative = 1;
    _restPositive = -1;
}


bool WeighterBase::Prun( Weight current, Weight minWeight, Weight maxWeight ) const
{
    if (_restPositive + current < minWeight)
        return true;

    if (_restNegative + current > maxWeight)
        return true;

    return false;
}


bool WeighterBase::Prun( DualizationNode& node, Weight minWeight, Weight maxWeight ) const
{
    if (node.pruned)
        return true;

    if (node.IsCover())
        return node.pruned = node.weight < minWeight || node.weight > maxWeight;

    return node.pruned = Prun(node.weight, minWeight, maxWeight);
}


bool WeighterBase::Prun( DualizationNode& node )
{
    return Prun(node, _minWeight, _maxWeight);
}


void WeighterBase::CalculateRestWeights()
{
    _restNegative = 0;
    _restPositive = 0;
        
    if (_weightSumCount <= 0)
    {
        for(auto w:_weights)
            IncRestWeight(w);

        if (GetDefaultWeight() < 0)
            _restNegative = -std::numeric_limits<Weight>::max();
        else if (GetDefaultWeight() > 0)
            _restPositive = std::numeric_limits<Weight>::max();
        
    }
    else
    {
        for (int j = 0; j < _weightSumCount; ++j)
            IncRestWeight(GetWeight(j));
    }
}



Weight WeighterBase::GetRestNegative() const
{
    return _restNegative;
}


Weight WeighterBase::GetRestPositive() const
{
    return _restPositive;
}


void WeighterBase::SetMaxWeight( Weight w )
{
    _maxWeight = w;
}


void WeighterBase::SetMinWeight( Weight w )
{
    _minWeight = w;
}


void WeighterBase::Reset()
{
    InvalidateRestWeights();
    _maxWeight = std::numeric_limits<Weight>::max();
    _minWeight = -_maxWeight;        
    _weights.clear();
    _weightSumCount = 0;
}


Weight WeighterBase::GetDefaultWeight()
{
    if (_weights.empty())
        return 0;

    // Последний элемент в списке выступает в качестве веса по умолчанию
    return _weights.back();
}


Weight WeighterBase::GetWeight( int i )
{
    if (i < 0)
        return 0;

    if (i >= (int)_weights.size())
        return GetDefaultWeight();

    return _weights[i];
}


void WeighterBase::SetWeights( Weights weights )
{
    _weights = std::move(weights);
}


void WeighterBase::WeightInnerNode( DualizationNode &node )
{
    if (! IsRestWeightsValid())
        CalculateRestWeights();

    switch (node.type)
    {
    case NodeType::NewNode:         
        WeightNewNode(node);
        Prun(node);
        break;

    case NodeType::BacktrackNode:        
        WeightBacktrackNode(node);
        break;

    }
}


WeighterBase::WeighterBase()
{
    Reset();
}


void WeighterBase::Call( DualizationNode & node )
{
    WeightInnerNode(node);
}

void WeighterBase::IncRestWeight( Weight w )
{
    if (w > 0)
        _restPositive += w;
    if (w < 0)
        _restNegative += w;
}

void WeighterBase::DecRestWeight( Weight w )
{
    if (w > 0)
        _restPositive -= w;
    if (w < 0)
        _restNegative -= w;
}