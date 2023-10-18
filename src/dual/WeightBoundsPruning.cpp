#include "WeightBoundsPruning.h"
#include <limits>

void WeightBoundsPruningCallback::Call(DualizationNode& node) {
    switch (node.type) {
        case NodeType::BacktrackNode: {
            Weight w = GetWeight(node.lastColumn);
            node.weight -= w;
            if (!IsDefaultWeight(node.lastColumn))
                IncRestWeight(w);
        } break;

        case NodeType::NewNode: {
            Weight w = GetWeight(node.lastColumn);
            node.weight += w;
            if (!IsDefaultWeight(node.lastColumn))
                DecRestWeight(w);
            PrunInnerNode(node);
        } break;

        case NodeType::CoverNode:
            PrunCoverNode(node);
    }
}

Weight WeightBoundsPruningCallback::GetWeight(int j) const {
    return IsDefaultWeight(j) ? _defaultWeight : _weights[j];
}

void WeightBoundsPruningCallback::SetWeights(Weights weights) {
    _weights = std::move(weights);
    _restNegative = 0;
    _restPositive = 0;

    for (auto w : _weights)
        IncRestWeight(w);
}

void WeightBoundsPruningCallback::SetEpsilon(Weight eps) {
    _epsilon = eps;
}

void WeightBoundsPruningCallback::TargetToMax() {
    _dynamicMinBound = true;
}

void WeightBoundsPruningCallback::FixMinBound(Weight bound) {
    _minWeight = bound;
    _dynamicMinBound = false;
}

void WeightBoundsPruningCallback::TargetToMin() {
    _dynamicMaxBound = true;
}

void WeightBoundsPruningCallback::FixMaxBound(Weight bound) {
    _dynamicMaxBound = false;
    _maxWeight = bound;
}

void WeightBoundsPruningCallback::Reset() {
    _weights.clear();
    _defaultWeight = 0;
    _dynamicMaxBound = false;
    _dynamicMinBound = false;
    _epsilon = 0;
    _minWeight = -std::numeric_limits<Weight>::max();
    _maxWeight = std::numeric_limits<Weight>::max();
    _restNegative = 0;
    _restPositive = 0;
}

WeightBoundsPruningCallback::WeightBoundsPruningCallback() {
    Reset();
}

void WeightBoundsPruningCallback::SetDefaultWeight(Weight w) {
    _defaultWeight = w;
}

void WeightBoundsPruningCallback::PrunCoverNode(DualizationNode& node) {
    node.Prun(node.weight > _maxWeight);
    node.Prun(node.weight < _minWeight);

    if (_dynamicMaxBound) {
        auto maxBound = node.weight + _epsilon;
        if (maxBound < _maxWeight)
            _maxWeight = maxBound;
    }

    if (_dynamicMinBound) {
        auto minBound = node.weight - _epsilon;
        if (minBound > _minWeight)
            _minWeight = minBound;
    }
}

bool WeightBoundsPruningCallback::IsDefaultWeight(int j) const {
    return j < 0 || j >= (int) _weights.size();
}

void WeightBoundsPruningCallback::PrunInnerNode(DualizationNode& node) {
    if (!node.pruned && _defaultWeight >= 0)
        node.Prun(node.weight + _restNegative > _maxWeight);

    if (!node.pruned && _defaultWeight <= 0)
        node.Prun(node.weight + _restPositive < _minWeight);
}

void WeightBoundsPruningCallback::IncRestWeight(Weight w) {
    if (w < 0)
        _restNegative += w;
    else if (w > 0)
        _restPositive += w;
}

void WeightBoundsPruningCallback::DecRestWeight(Weight w) {
    if (w < 0)
        _restNegative -= w;
    else if (w > 0)
        _restPositive -= w;
}