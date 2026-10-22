#include <dual/runc_executor.h>

#include <algorithm>
#include <cstdint>
#include <new>
#include <vector>

#include <dual/runc.h>

namespace {

// Object behind a DualizerHandler. The input is kept as plain rows and the bit matrix is rebuilt
// before the next enumeration, so rows may come in any order and grow in any way.
struct RuncDualizer {
    dual::Runc::RuncGlobalState state;
    std::vector<std::vector<int32_t>> rows;
    bool matrix_dirty = true;

    void buildMatrix() {
        int n = 0;
        for (auto const& row : rows)
            for (auto j : row)
                n = std::max(n, j + 1);

        // bit_matrix counts blocks in int
        if (static_cast<uint64_t>(rows.size()) * dual::bit_chunk::block_for_bits(n) > INT32_MAX)
            throw std::bad_alloc();

        // Fixed row stride, bits are set inside it: no reallocation while filling
        dual::bit_matrix matrix;
        matrix.zero(static_cast<int>(rows.size()), n);
        for (size_t i = 0; i < rows.size(); ++i)
            for (auto j : rows[i])
                matrix.get_row(static_cast<int>(i)).set(j);

        state.setMatrix(std::move(matrix));
        matrix_dirty = false;
    }
};

}  // namespace

extern "C" {

DualizerHandler init_dualizer(void) {
    return new (std::nothrow) RuncDualizer();
}

void release_dualizer(DualizerHandler dualizer) {
    delete static_cast<RuncDualizer*>(dualizer);
}

int32_t set_input_row(DualizerHandler dualizer, int32_t row_num, int32_t ones_num, const int32_t* ones) {
    if (!dualizer || row_num < 0 || row_num == INT32_MAX || ones_num < 0 || (ones_num > 0 && !ones))
        return RUNC_ERROR_INVALID_ARGUMENT;

    for (int32_t k = 0; k < ones_num; ++k) {
        if (ones[k] < 0 || ones[k] == INT32_MAX)
            return RUNC_ERROR_INVALID_ARGUMENT;
    }

    auto& d = *static_cast<RuncDualizer*>(dualizer);
    try {
        if (d.rows.size() <= static_cast<size_t>(row_num))
            d.rows.resize(static_cast<size_t>(row_num) + 1);
        d.rows[row_num].insert(d.rows[row_num].end(), ones, ones + ones_num);
    }
    catch (...) {
        return RUNC_ERROR_INTERNAL;
    }
    d.matrix_dirty = true;
    d.state.resetDualize();
    return RUNC_OK;
}

int32_t enumerate_covers(DualizerHandler dualizer, uint32_t buffer_size, uint32_t* covers) {
    if (!dualizer || !covers)
        return RUNC_ERROR_INVALID_ARGUMENT;

    auto& d = *static_cast<RuncDualizer*>(dualizer);
    try {
        if (d.matrix_dirty)
            d.buildMatrix();

        auto const& matrix = static_cast<dual::Runc::RuncGlobalState const&>(d.state).get_matrix();
        if (buffer_size < static_cast<uint64_t>(matrix.width()) + 1)
            return RUNC_ERROR_BUFFER_TOO_SMALL;

        // At most one record per word, so the count fits into int32_t
        auto size = std::min<uint32_t>(buffer_size, INT32_MAX);
        return static_cast<int32_t>(d.state.dualize(size, covers));
    }
    catch (...) {
        d.matrix_dirty = true;
        return RUNC_ERROR_INTERNAL;
    }
}
}
