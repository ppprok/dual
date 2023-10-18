
#include "PUNC.h"
#include "containers.h"

namespace PUNC {

bool GlobalState::DoDualize(int level) {
    auto& child = GetLocalState(level + 1);
    auto& current = GetLocalState(level);
    //assert(child);
    //assert(current);

    int r = FindTopUncoveredRow(current);
    if (r == _matrix.height())
        return DoCoverCallback();

    if (!CollectRestCols(current))
        return true;

    child.usedColumns.assign(current.usedColumns);
    CollectColsOrder(current.restColumns, current.columnsOrder);

    for (auto& cc : current.columnsOrder) {
        int j = cc.order;
        if (AddColumn(j)) {
            InitChildState(child, current, j);
            if (!DoDualize(level + 1))
                return false;
        }

        Backtrack();
        child.usedColumns.set(j, false);

        if (_currentNode.stoped)
            break;
    }

    return !_currentNode.stoped;
}

LocalState& GlobalState::GetLocalState(int level) {
    while (level >= (int) _states.size()) {
        _states.emplace_back();
        InitState(_states.back());
    }

    return _states[level];
}

void GlobalState::Dualize() {
    //_states.clear();
    int n = _matrix.width();
    int m = _matrix.height();
    ResetCurrentNode(n);
    _tempRow = bit_vector(n);

    int q = std::min(n, m) + 1;
    _states.reserve(q);
    DoDualize(0);
}

int GlobalState::FindTopUncoveredRow(LocalState& state) {
    int i = state.topRow;
    int s = _matrix.height();

    bit_mask mask(state.usedColumns);

    for (; i < s; ++i) {
        int t = GetRowOrder(i);

        int c = mask.count(_matrix.get_row(t), 1);
        if (c == 0)
            break;

        if (c == 1) {
            int j = mask.find_last(_matrix.get_row(t));
            state.supportRows.push_back(std::make_pair(j, t));
        }
    }

    return state.topRow = i;
}

void GlobalState::InitState(LocalState& state) {
    state.topRow = 0;
    state.supportRows.clear();

    int m = _matrix.height();
    int n = _matrix.width();

    state.supportRows.reserve(m);
    state.usedColumns = bit_vector(n);
    state.restColumns = bit_vector(n);
}

bool GlobalState::CollectRestCols(LocalState& state) {
    assert(state.topRow >= 0 && state.topRow < (int) _matrix.height());
    state.restColumns.assign(_matrix.get_row(GetRowOrder(state.topRow)));

    sort(state.supportRows);

    int j = -1;

    for (auto r : state.supportRows) {
        if (j != r.first) {
            if (j >= 0 && !_tempRow.empty()) {
                state.restColumns.assign_sub(state.restColumns, _tempRow);
                if (state.restColumns.empty())
                    return false;
            }

            j = r.first;
            _tempRow.assign(_matrix.get_row(r.second));
            continue;
        }

        if (!_tempRow.empty()) {
            _tempRow.assign_and(_matrix.get_row(r.second), _tempRow);
        }
    }

    if (!_tempRow.empty() && !state.restColumns.empty()) {
        state.restColumns.assign_sub(state.restColumns, _tempRow);
    }

    return state.restColumns.any();
}

void GlobalState::InitChildState(LocalState& child, LocalState const& parent, int j) {
    child.topRow = parent.topRow + 1;
    child.usedColumns.set(j);
    child.supportRows.clear();

    for (auto r : parent.supportRows) {
        if (!_matrix.get(r.second, j))
            child.supportRows.push_back(r);
    }

    child.supportRows.push_back(std::make_pair(j, GetRowOrder(parent.topRow)));
}

void GlobalState::SetRowsOrder(Weights const& weights) {
    SetOrder(weights, _matrix.height(), _rowsOrder);
}

void GlobalState::SetColsOrder(Weights weights) {
    _colsWeights = std::move(weights);
}

void GlobalState::SetOrder(Weights const& weights, int n, OrderByWeights& order) {
    order.resize(n);
    for (int i = 0; i < n; ++i) {
        order[i].order = i;
        order[i].weight = (int) weights.size() > i ? weights[i] : 0;
    }

    Sort(order);
    return;
}

void GlobalState::CollectColsOrder(bit_chunk bits, OrderByWeights& order) {
    order.clear();

    for (int j = bits.find_first(); j >= 0; j = bits.find_next(j)) {
        order.emplace_back(j, j < (int) _colsWeights.size() ? _colsWeights[j] : 0);
    }

    Sort(order);
}

void GlobalState::Sort(OrderByWeights& order) {
    std::random_shuffle(order.begin(), order.end());
    std::sort(order.begin(), order.end(), [](OrderByWeight const& o1, OrderByWeight const& o2) {
        return o1.weight < o2.weight;
    });
}

int GlobalState::GetRowOrder(int i) const {
    return _rowsOrder.empty() ? i : _rowsOrder[i].order;
}

}  // namespace PUNC