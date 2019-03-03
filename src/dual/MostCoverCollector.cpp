#include "MostCoverCollector.h"


void MostCoverCollector::Call( DualizationNode& node )
{
    if (! node.IsCover())
        return;
    
    if (Weighter)
    {
        if (Weighter->Prun(node))
            return;        
    }

    _covers.emplace_back(node.columns, node.weight);
        
    if ((int)_covers.size() <= Options.TrimSize)
        return;

    Trim(Options.MostCount);

    if (Weighter && Options.TrimOrder != 0 && ! _covers.empty())
    {
        if (Options.TrimOrder < 0)
        {
            Weighter->SetMaxWeight(_covers.back().weight);
        }
        else
        {
            Weighter->SetMinWeight(_covers.back().weight);
        }
    }

}


void MostCoverCollector::Trim(int size)
{   
    if ((int)_covers.size() <= size)
        return;

    if (Options.RandomShuffle)
        std::random_shuffle(_covers.begin(), _covers.end());

    if (Options.TrimOrder < 0)
    {
        std::sort(_covers.begin(), _covers.end(), 
        [](WeightedCover const& c1, WeightedCover const& c2)
            {
                if (c1.weight == c2.weight)
                    return c1.columns.size() < c2.columns.size();
                return c1.weight < c2.weight;
            });
    }
    
    if (Options.TrimOrder > 0)
    {
        std::sort(_covers.begin(), _covers.end(), 
        [](WeightedCover const& c1, WeightedCover const& c2)
            {
                if (c1.weight == c2.weight)
                    return c1.columns.size() < c2.columns.size();
                return c1.weight > c2.weight;
            });
    }
    
    _covers.resize(size);    
}

void MostCoverCollector::Trim()
{
    Trim(Options.MostCount);
}

MostCoverCollector::MostCoverCollector( WeighterBase* weighter /*= 0*/ )
{
    Weighter = weighter;
    _counter = 0;
}

void BitCoverCollectorCallback::Call( DualizationNode& node )
{
    if (node.type != NodeType::CoverNode)
        return;
        
    int i = _covers.height();
    for (int j:node.columns)
    {
        _covers.set(i, j);
    }

    _weights.push_back(node.weight);
}

bit_matrix const& BitCoverCollectorCallback::GetCovers() const
{
    return _covers;
}

bit_matrix BitCoverCollectorCallback::MoveCovers()
{
    return std::move(_covers);
}

void BitCoverCollectorCallback::Reset()
{
    _covers.zero(0, 0);
    _weights.clear();
}

Weights const& BitCoverCollectorCallback::GetWeights() const
{
    return _weights;
}

Weights BitCoverCollectorCallback::MoveWeights()
{
    return std::move(_weights);
}