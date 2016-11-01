#include "stdafx.h"

#include "CoverCollector.h"


void CoverCollector::Call( DualizationNode& node )
{
    if (! node.IsCover())
        return;

    _covers.emplace_back(node.columns, node.weight);
    if (MaxCount > 0 && (int)_covers.size() >= MaxCount)
        node.stoped = true;
}

void CoverCollector::Reserve( int size )
{
    _covers.reserve(size);
}

void CoverCollector::Reset()
{
    _covers.clear();
}

WeightedCovers CoverCollector::MoveCovers()
{
    return std::move(_covers);
}

WeightedCovers const& CoverCollector::GetCovers() const
{
    return _covers;
}
