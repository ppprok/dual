#include <dual/runc.h>

#include <iostream>

#include <dual/dualization_backtrack_algorithm_base.h>

#include "bool_matrix_preparation.h"

namespace {

// Installs a temporary CoverCallback and restores the caller's one on scope exit
class CoverCallbackGuard {
public:
    CoverCallbackGuard(DualizationBacktrackAlgorithmBase& algorithm, IDualizationCallback* callback) :
            algorithm_(algorithm), saved_(algorithm.CoverCallback) {
        algorithm_.CoverCallback = callback;
    }
    ~CoverCallbackGuard() {
        algorithm_.CoverCallback = saved_;
    }
    CoverCallbackGuard(CoverCallbackGuard const&) = delete;
    CoverCallbackGuard& operator=(CoverCallbackGuard const&) = delete;

private:
    DualizationBacktrackAlgorithmBase& algorithm_;
    IDualizationCallback* saved_;
};

}  // namespace

bool Runc::RuncLocalState::SetUncoveredSupportRows(int col, const marked_ranges& groups, const rows& support) {

    for (auto g : groups) {
        bool b = AddUncoveredSupportRows(col, g.get_value(), g.begin_in(support), g.end_in(support));
        if (!b)
            return false;
    }

    return true;
}

bool Runc::RuncLocalState::SetUncoveredSupportRows(const RuncLocalState& parent, int col) {
    return SetUncoveredSupportRows(col, parent.supportGroups_, parent.support_);
}

bool Runc::RuncLocalState::AddUncoveredSupportRows(int col, int l, rows_citerator begin, rows_citerator end) {
    int s = support_.size();
    temp_row_.assign(getRestCols());

    for (; begin != end; ++begin) {
        if (col >= 0 && begin->get(col))
            continue;

        auto r = *begin;

        if (!temp_row_.empty())
            temp_row_.assign_and(r, temp_row_);

        if (temp_row_.empty()) {
            getRestCols().trim(r);
            if (r.empty()) {
                support_.resize(s);
                return true;
            }
        }

        support_.push_back(r);
    }

    if (s == support_.size())
        return false;

    if (!temp_row_.empty())
        restCols_.assign_sub(restCols_, temp_row_);

    supportGroups_.push_back(marked<range>(range(s, support_.size()), l));

    return true;
}

bool Runc::RuncLocalState::SetUncovered(int col, rows_citerator begin, rows_citerator end) {
    uncovered_.resize(end - begin);
    auto uend = uncovered_.end();
    auto ubeg = uncovered_.begin();

    for (; begin != end; ++begin) {
        if (begin->get(col)) {
            *--uend = *begin;
            continue;
        }

        auto r = *begin;

        getRestCols().trim(r);

        if (r.empty())
            return false;

        *ubeg++ = r;
    }

    if (!AddUncoveredSupportRows(-1, col, uend, uncovered_.end()))
        return false;

    uncovered_.resize(ubeg - uncovered_.begin());
    return true;
}

bool Runc::RuncLocalState::SetUncovered(RuncLocalState const& parent, int col) {
    return SetUncovered(col, parent.uncovered_.begin(), parent.uncovered_.end());
}

Runc::RuncLocalState::AddResult Runc::RuncLocalState::AddColumn(RuncLocalState const& l, int col) {
    support_.clear();
    supportGroups_.clear();
    uncovered_.clear();

    restCols_.assign(l.restCols_);

    if (!SetUncovered(l, col))
        return Uncoverable;

    if (!SetUncoveredSupportRows(l, col))
        return Uncompatable;

    return IsCover() ? Cover : Inner;
}

void Runc::RuncLocalState::RemoveColumn(int j) {
    restCols_.set(j, 0);
}

bit_chunk Runc::RuncLocalState::GetMinUncovered(int mc /*= 0*/) const {
    bit_chunk row;
    int count = INT_MAX;

    for (auto r : uncovered_) {
        int c = getRestCols().count(r, count);

        if (c < count) {
            if (c <= mc)
                return r;

            count = c;
            row = r;
        }
    }

    return row;
}

bit_chunk Runc::RuncLocalState::CollectRestCols() {
    restCols_.clear();

    for (auto r : uncovered_)
        restCols_.assign_or(restCols_, r);

    return restCols_;
}

void Runc::RuncLocalState::RemoveDependentRows() {
    BoolMatrixPreparation::RemoveDependentRows(uncovered_, getRestCols());
}

bit_chunk Runc::RuncLocalState::GetUndepCols() {
    RemoveDependentRows();

    temp_row_.clear();
    for (auto const& uncovered : uncovered_) {
        temp_row_.assign_or(uncovered, temp_row_);
    }
    restCols_.assign_and(restCols_, temp_row_);
    return restCols_;
}

bit_chunk Runc::RuncLocalState::collectCols(AlgorithmType alg) {
    switch (alg) {
        case Runc::OPT:
            return GetUndepCols();
        case Runc::RUNC:
            return GetFrontUncovered();
        case Runc::RUNCM:
            return GetMinUncovered(1);
    }
    return bit_chunk();
}

bit_chunk Runc::RuncLocalState::GetFrontUncovered() const {
    return uncovered_.front();
}

bit_mask Runc::RuncLocalState::getRestCols() const {
    return restCols_;
}

bool Runc::RuncLocalState::IsCover() const {
    return uncovered_.empty();
}

Runc::RuncLocalState::RuncLocalState(size_t m, size_t n) {
    if (m > 0 && n > 0) {
        uncovered_.reserve(m);
        support_.reserve(m);
        supportGroups_.reserve(std::min(m, n));
        restCols_ = bit_vector(n);
        temp_row_ = bit_vector(n);
    }
}
const bit_chunk& Runc::RuncLocalState::getCollectedCols() const {
    return collectedCols_;
}
void Runc::RuncLocalState::setCollectedCols(bit_chunk collectedCols) {
    collectedCols_ = collectedCols;
}

void Runc::RuncGlobalState::doDualize() {

    do {
    } while (doStep());
}

Runc::RuncLocalState& Runc::RuncGlobalState::getLocalState(std::size_t state_num) {
    if (state_num >= _states.size()) {
        auto m = _matrix.height();
        auto n = _matrix.width();
        while (state_num >= _states.size())
            _states.emplace_back(m, n);
    }
    return _states[state_num];
}

void Runc::RuncGlobalState::dualize() {
    startDualize();
    try {
        doDualize();
    }
    catch (...) {
        resetDualize();
        throw;
    }
}

Runc::RuncGlobalState::RuncGlobalState(AlgorithmType type /*= AlgorithmType::RUNCM*/) : _type(type) {}

void Runc::RuncGlobalState::startDualize() {
    int n = _matrix.width();
    int m = _matrix.height();
    int q = std::min(m, n);

    _states.clear();
    state_depth_ = 0;
    resetCurrentNode(std::max(q, 0));

    if (m <= 0) {
        // No rows: the empty set is the only minimal cover
        enumeration_ = Enumeration::EmptyCover;
        return;
    }

    for (auto const& r : _matrix.get_rows()) {
        if (!r.begin || r.find_first() < 0) {
            // A zero row cannot be covered
            enumeration_ = Enumeration::Finished;
            return;
        }
    }

    _states.reserve(q + 1);
    auto& root = getLocalState(0);
    root.uncovered_.assign(_matrix.get_rows().begin(), _matrix.get_rows().end());
    root.CollectRestCols();
    root.setCollectedCols(root.collectCols(_type));
    enumeration_ = Enumeration::Running;
}

void Runc::RuncGlobalState::resetDualize() {
    _states.clear();
    state_depth_ = 0;
    enumeration_ = Enumeration::NotStarted;
}

bool Runc::RuncGlobalState::isDualizeFinished() const {
    return enumeration_ == Enumeration::Finished;
}

void Runc::RuncGlobalState::onMatrixChanged() {
    resetDualize();
}

bool Runc::RuncGlobalState::doStep() {

    if (enumeration_ == Enumeration::EmptyCover) {
        enumeration_ = Enumeration::Finished;
        doCoverCallback();
        return false;
    }

    if (enumeration_ != Enumeration::Running)
        return false;

    auto* current = &_states[state_depth_];
    int j = current->getRestCols().find_last(current->getCollectedCols());
    if (j >= 0) {
        current->RemoveColumn(j);
        if (addColumn(j)) {
            auto& child = getLocalState(state_depth_ + 1);
            current = &_states[state_depth_];  // getLocalState may have reallocated _states
            auto add_result = child.AddColumn(*current, j);
            switch (add_result) {
                case Runc::RuncLocalState::Inner:
                    ++state_depth_;
                    child.setCollectedCols(child.collectCols(_type));
                    return true;
                case Runc::RuncLocalState::Cover:
                    doCoverCallback();
                    break;
                default:
                    break;
            }
        }
    } else if (state_depth_ > 0) {
        --state_depth_;
    } else {
        enumeration_ = Enumeration::Finished;
        return false;
    }
    backtrack();
    if (_currentNode.stoped) {
        // A callback stopped the enumeration: treat it as finished
        enumeration_ = Enumeration::Finished;
        return false;
    }
    return true;
}
std::vector<Columns> Runc::RuncGlobalState::dualize(size_t max_count) {
    if (enumeration_ == Enumeration::NotStarted) {
        startDualize();
    }

    std::vector<Columns> result;
    result.reserve(std::min<size_t>(max_count, 1024));
    auto callback = dualization_callback([&result](DualizationNode& node) {
        if (node.IsCover()) {
            result.push_back(node.columns);
        }
    });
    CoverCallbackGuard guard(*this, callback.get());
    try {
        while (result.size() < max_count) {
            if (!doStep())
                break;
        }
    }
    catch (...) {
        resetDualize();
        throw;
    }
    return result;
}
size_t Runc::RuncGlobalState::dualize(uint32_t buffer_size, uint32_t* buffer) {
    size_t n = _matrix.width();
    if (!buffer || buffer_size < n + 1)
        return 0;

    if (enumeration_ == Enumeration::NotStarted) {
        startDualize();
    }

    size_t count = 0;
    auto callback = dualization_callback([&buffer_size, &buffer, &count](DualizationNode& node) {
        if (!node.IsCover())
            return;

        Columns& cover = node.columns;
        buffer[0] = cover.size();
        ++buffer;
        --buffer_size;

        std::copy(cover.begin(), cover.end(), buffer);
        buffer += cover.size();
        buffer_size -= cover.size();
        ++count;
    });
    CoverCallbackGuard guard(*this, callback.get());
    try {
        while (buffer_size >= n + 1) {
            if (!doStep())
                break;
        }
    }
    catch (...) {
        resetDualize();
        throw;
    }
    return count;
}
