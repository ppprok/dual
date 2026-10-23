
#include <dual/dualization_backtrack_algorithm_base.h>

#include <dual/integers_file_reader.h>

namespace dual {

void DualizationBacktrackAlgorithmBase::loadMatrix(std::string const& filename) {
    setMatrix(IntegersFileReader(filename).ReadBitMatrix());
}

void DualizationBacktrackAlgorithmBase::doInnerCallback() {
    if (InnerCallback)
        InnerCallback->Call(_currentNode);
}

bool DualizationBacktrackAlgorithmBase::doCoverCallback() {
    _currentNode.type = NodeType::CoverNode;
    if (CoverCallback)
        CoverCallback->Call(_currentNode);
    return !_currentNode.stoped;
}

bool DualizationBacktrackAlgorithmBase::addColumn(int j) {
    _currentNode.AddColumn(j);
    doInnerCallback();
    return !_currentNode.stoped && !_currentNode.pruned;
}

void DualizationBacktrackAlgorithmBase::backtrack() {
    _currentNode.Backtrack();
    doInnerCallback();
}

void DualizationBacktrackAlgorithmBase::resetCurrentNode(int n /*= 0*/) {
    _currentNode.Reset(n);
}

void DualizationBacktrackAlgorithmBase::setRestrictions(bit_matrix const& restrictions) {
    int n = restrictions.width();
    _restrictions.zero(n, n);
    for (auto r : restrictions.get_rows()) {
        for (int j = r.find_first(); j >= 0; j = r.find_next(j)) {
            bit_chunk& row = _restrictions.get_row(j);
            row.assign_or(row, r);
        }
    }
}

void DualizationBacktrackAlgorithmBase::loadRestrictions(std::string const& filename) {
    setRestrictions(IntegersFileReader(filename).ReadBitMatrix());
}

void DualizationBacktrackAlgorithmBase::setMatrix(bit_matrix matrix) {
    resetCurrentNode(matrix.width());
    _matrix = std::move(matrix);
    onMatrixChanged();
}

DualizationBacktrackAlgorithmBase::DualizationBacktrackAlgorithmBase() :
        InnerCallback(nullptr), CoverCallback(nullptr) {}
DualizationBacktrackAlgorithmBase::~DualizationBacktrackAlgorithmBase() {}
bit_matrix& DualizationBacktrackAlgorithmBase::mutable_matrix() {
    onMatrixChanged();
    return _matrix;
}
bit_matrix const& DualizationBacktrackAlgorithmBase::get_matrix() const {
    return _matrix;
}

}  // namespace dual
