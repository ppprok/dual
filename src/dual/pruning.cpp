#include <dual/pruning.h>

#include <cmath>

namespace dual {

CoveredRowsWeighter::CoveredRowsWeighter() {
    Reset();
}

void CoveredRowsWeighter::Reset() {
    _uncoveredRows.clear();
    _targetRows.clear();
    _coveredRows.clear();
    _pruner.Reset();
    _completion.Clear();
}

void CoveredRowsWeighter::Call(DualizationNode& node) {
    switch (node.type) {
        case NodeType::NewNode:
            _pruner.SaveCheckpoint(node);
            WeightNewNode(node);
            _pruner.PrunInnerNode(node);
            if (_completion.Enabled()) {
                _completion.Update(node);
                _pruner.PrunByCompletion(node, _completion);
            }
            break;

        case NodeType::BacktrackNode:
            WeightBacktrackNode(node);
            _pruner.RestoreCheckpoint(node);
            break;

        case NodeType::CoverNode:
            _pruner.PrunCoverNode(node);
            break;

        default:  // root and extra nodes do not change the weight
            break;
    }
}

void CoveredRowsWeighter::WeightNewNode(DualizationNode& node) {
    bit_chunk::get_bit_iterator j(node.lastColumn);

    for (int i = 0; i < (int) _uncoveredRows.size(); ++i) {
        int r = _uncoveredRows[i];
        Weight w = _pruner.GetWeight(r);

        if (!_targetRows[r].get(j))
            continue;

        CoveredRow ur = {r, node.lastColumn, w};
        _coveredRows.push_back(ur);

        node.weight += w;
        _pruner.DecRestWeight(w);

        _uncoveredRows[i] = _uncoveredRows.back();
        _uncoveredRows.pop_back();
        --i;
    }
}

void CoveredRowsWeighter::WeightBacktrackNode(DualizationNode& node) {
    while (!_coveredRows.empty()) {
        auto r = _coveredRows.back();
        if (r.j != node.lastColumn)
            break;

        _coveredRows.pop_back();
        _uncoveredRows.push_back(r.i);

        node.weight -= r.w;
        _pruner.IncRestWeight(r.w);
    }
}

void CoveredRowsWeighter::SetOptions(Options options) {
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

void CoveredRowsWeighter::EnableCompletionBounds(bit_matrix const& matrix) {
    _completion.SetMatrix(matrix);

    // A column gains at most the positive weights of the target rows it hits and loses at most
    // their negative weights; the gain is not additive (a row hit twice counts once)
    int n = _completion.Columns();
    Weights positive(n, 0), negative(n, 0);
    Weight magnitude = 0;
    for (int i = 0; i < (int) _targetRows.size(); ++i) {
        Weight w = _pruner.GetWeight(i);
        magnitude += std::fabs(w);
        auto const& row = _targetRows[i];
        if (w == 0 || !row.begin)
            continue;
        for (int j = row.find_first(); j >= 0 && j < n; j = row.find_next(j))
            (w > 0 ? positive : negative)[j] += w;
    }
    _completion.SetGains(positive, negative, false, magnitude, (int) _targetRows.size());
}

void CoveredRowsWeighter::FixMaxBound(Weight bound) {
    _pruner.FixMaxBound(bound);
}

void CoveredRowsWeighter::FixMinBound(Weight bound) {
    _pruner.FixMinBound(bound);
}

void CoveredRowsWeighter::TargetToMin() {
    _pruner.TargetToMin();
}

void CoveredRowsWeighter::TargetToMax() {
    _pruner.TargetToMax();
}

void CoveredRowsWeighter::SetEpsilon(Weight eps) {
    _pruner.SetEpsilon(eps);
}

}  // namespace dual
