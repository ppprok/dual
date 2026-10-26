#include <dual/weight_bounds_pruning.h>

#include <limits>

namespace dual {

void WeightCheckpoints::Save(DualizationNode const& node, State const& state) {
    // The parent of the new node is at depth columns.size() - 1; deeper states are stale
    size_t depth = node.columns.empty() ? 0 : node.columns.size() - 1;
    states_.resize(depth);
    valid_.resize(depth, 0);
    states_.push_back(state);
    valid_.push_back(1);
}

bool WeightCheckpoints::Restore(DualizationNode const& node, State& state) {
    size_t depth = node.columns.size();
    bool found = depth < states_.size() && valid_[depth];
    if (found)
        state = states_[depth];
    if (depth < states_.size()) {
        states_.resize(depth);
        valid_.resize(depth);
    }
    return found;
}

void WeightCheckpoints::Clear() {
    states_.clear();
    valid_.clear();
}

void WeightBoundsPruningCallback::Call(DualizationNode& node) {
    switch (node.type) {
        case NodeType::BacktrackNode: {
            Weight w = GetWeight(node.lastColumn);
            node.weight -= w;
            if (!IsDefaultWeight(node.lastColumn))
                IncRestWeight(w);
            RestoreCheckpoint(node);
        } break;

        case NodeType::NewNode: {
            SaveCheckpoint(node);
            Weight w = GetWeight(node.lastColumn);
            node.weight += w;
            if (!IsDefaultWeight(node.lastColumn))
                DecRestWeight(w);
            PrunInnerNode(node);
        } break;

        case NodeType::CoverNode:
            PrunCoverNode(node);
            break;

        default:  // root and extra nodes do not change the weight
            break;
    }
}

Weight WeightBoundsPruningCallback::GetWeight(int j) const {
    return IsDefaultWeight(j) ? _defaultWeight : _weights[j];
}

void WeightBoundsPruningCallback::SetWeights(Weights weights) {
    _weights = std::move(weights);
    _restNegative = 0;
    _restPositive = 0;
    _checkpoints.Clear();

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
    _checkpoints.Clear();
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

void WeightBoundsPruningCallback::SaveCheckpoint(DualizationNode const& node) {
    _checkpoints.Save(node, {node.weight, _restPositive, _restNegative});
}

void WeightBoundsPruningCallback::RestoreCheckpoint(DualizationNode& node) {
    WeightCheckpoints::State state{node.weight, _restPositive, _restNegative};
    if (!_checkpoints.Restore(node, state))
        return;
    node.weight = state.weight;
    _restPositive = state.restPositive;
    _restNegative = state.restNegative;
}

}  // namespace dual
