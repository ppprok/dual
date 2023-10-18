
#include <algorithm>

#include "AOT1.h"
#include "BoolMatrixPreparation.h"
#include "bit_mask.h"

// Инициализировать по глобальному состоянию
void AOT1::LocalState::Init(GlobalState& g) {
    int m = g._matrix.height();
    int n = g._matrix.width();
    _uncovered.reserve(m);
    _competing.reserve(m);

    g._tempMatrix.add_rows(1, n);
    _restCols = g._tempMatrix.get_rows().back();
}

// Добавить столбец
AOT1::LocalState::AddResult AOT1::LocalState::AddItem(GlobalState& g, LocalState const& l, int i, int col) {
    _competing.clear();
    _uncovered.clear();
    _restCols.assign_sub(l._restCols, l._uncovered[i]);

    bit_chunk::get_bit_iterator j(col);

    for (auto r : l._competing) {
        if (!r.get(j))
            _competing.push_back(r);
    }

    for (int t = 0; t < i; ++t) {
        auto r = l._uncovered[t];
        if (r.get(j))
            _competing.push_back(r);
        else
            _uncovered.push_back(r);
    }

    int s = l._uncovered.size();
    for (int t = i; t < s; ++t) {
        auto r = l._uncovered[t];
        if (!r.get(j))
            _uncovered.push_back(r);
    }

    if (_uncovered.empty() && !_competing.empty())
        return Uncoverable;

    return IsCover() ? Cover : Inner;
}

// Удалить столбец
void AOT1::LocalState::RemCol(int j) {
    _restCols.set(j, 0);
}

// Найти минимальную непокрытую строку
bit_chunk AOT1::LocalState::GetMinUncovered(int mc /*= 0*/) const {
    bit_chunk row;
    int count = INT_MAX;

    bit_mask mask(_restCols);
    for (auto r : _competing) {
        int c = mask.count(r, count);

        if (c < count) {
            if (c <= mc)
                return r;

            count = c;
            row = r;
        }
    }

    for (auto r : _uncovered) {
        int c = mask.count(r, count);

        if (c < count) {
            if (c <= mc)
                return r;

            count = c;
            row = r;
        }
    }

    return row;
}

// Удалить нулевые столбцы
bit_chunk AOT1::LocalState::CollectRestCols() {
    _restCols.clear();

    for (auto r : _uncovered)
        _restCols.assign_or(_restCols, r);

    return _restCols;
}

// Удалить нижние охватывающие строки
void AOT1::LocalState::RemoveDependentRows() {
    BoolMatrixPreparation::RemoveDependentRows(_uncovered, _restCols);
}

AOT1::LocalState& AOT1::LocalState::operator=(LocalState&& s) {
    if (&s == this)
        return *this;

    _uncovered = std::move(s._uncovered);
    _competing = std::move(s._competing);
    _restCols = s._restCols;

    return *this;
}

bit_chunk AOT1::LocalState::GetNonZeroCols(GlobalState& g) {
    g._tempRow.clear();
    for (auto r : _uncovered) {
        g._tempRow.assign_or(r, g._tempRow);
    }
    _restCols.assign_and(_restCols, g._tempRow);

    return _restCols;
}

bit_chunk AOT1::LocalState::GetCols(GlobalState& g, AOT1::AlgorithmType alg) {
    switch (alg) {
        case AOT1::AO2:
        case AOT1::AO2K:
        case AOT1::AO2M:
            RemoveDependentRows();
    }

    switch (alg) {
        case AOT1::AO1:
        case AOT1::AO2:
            return _restCols;  //GetNonZeroCols(g);
        case AOT1::AO1K:
        case AOT1::AO2K:
            return GetFrontUncovered();
        case AOT1::AO1M:
        case AOT1::AO2M:
            return GetMinUncovered();
    }

    return _restCols;
}

bit_chunk AOT1::LocalState::GetFrontUncovered() const {
    if (!_competing.empty())
        return _competing.front();
    return _uncovered.front();
}

bit_chunk AOT1::LocalState::GetRestCols() const {
    return _restCols;
}

bool AOT1::LocalState::IsCover() const {
    return _uncovered.empty() && _competing.empty();
}

/*bool AOT1::LocalState::SetUncovered( GlobalState& g, LocalState const& parent, int col )
{
    return SetUncovered(g, col, parent._uncovered.begin(), parent._uncovered.end());
}*/

AOT1::LocalState::LocalState(LocalState&& s) {
    *this = std::move(s);
}

bool AOT1::GlobalState::DoDualize(int stateNum) {
    auto current = GetLocalState(stateNum);
    assert(current);
    auto child = GetLocalState(stateNum + 1);
    assert(child);

    auto cols = current->GetCols(*this, _type);
    int s = current->_uncovered.size();

    bit_mask mask(current->GetRestCols());
    while (!_currentNode.stoped) {
        int j = mask.find_last(cols);
        if (j < 0)
            break;

        current->RemCol(j);

        if (AddColumn(j)) {
            bit_chunk::get_bit_iterator jt(j);

            for (int i = 0; i < s; ++i) {
                if (!current->_uncovered[i].get(jt))
                    continue;

                auto r = child->AddItem(*this, *current, i, j);

                if (r == LocalState::Inner) {
                    if (!DoDualize(stateNum + 1))
                        return false;
                } else if (r == LocalState::Cover) {
                    if (!DoCoverCallback())
                        return false;
                }
            }
        }

        Backtrack();
    }

    return !_currentNode.stoped;
}

/*Positions const& AOT1::GlobalState::GetCurrentCols()
{
    /*_cols.clear();
    for (int s = 1; s <= _currentState; ++s)
    {
        int j = _states[s]._col;
        if (j >=0)
            _cols.push_back(j);
    } 

    return _cols;
}*/

AOT1::LocalState* AOT1::GlobalState::GetLocalState(int stateNum) {
    assert(stateNum >= 0);
    while (stateNum >= (int) _states.size()) {
        _states.emplace_back();
        _states.back().Init(*this);
    }

    return &_states[stateNum];
}

void AOT1::GlobalState::Dualize() {
    int n = _matrix.width();
    int m = _matrix.height();
    int q = std::min(m, n);

    if (q <= 0)
        return;

    _tempMatrix.reserve(q + 2, n);
    _tempMatrix.add_rows(1, n);
    _tempRow = _tempMatrix.get_row(0);

    _states.clear();
    _states.reserve(q + 1);

    auto root = GetLocalState(0);
    root->_uncovered.assign(_matrix.get_rows().begin(), _matrix.get_rows().end());
    root->CollectRestCols();

    ResetCurrentNode(q);
    DoDualize(0);
}
