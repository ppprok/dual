#include "RuncExecutor.h"
#include "Counters.h"
#include "RUNC.h"

extern "C"

        class DualizerState {
public:
    Runc::GlobalState state;
    Counters counters;
    bit_matrix input_matrix;

    DualizerState(int32_t m, int32_t n) {
        state.InnerCallback = &counters;
        state.CoverCallback = &counters;
        input_matrix.add_rows(m, n);
    };

    void set_input_one(int32_t i, int32_t j) {
        input_matrix.set(i, j);
    }

    void dualize() {
        state.SetMatrix(std::move(input_matrix));
        state.Dualize();
    }
};

DualizerHandler init_dualizer(int32_t m, int32_t n) {
    return new DualizerState(m, n);
}

void release_dualizer(DualizerHandler dualizer) {
    delete reinterpret_cast<DualizerState*>(dualizer);
}

void dualize(DualizerHandler dualizer) {
    reinterpret_cast<DualizerState*>(dualizer)->dualize();
}

void set_input_row(DualizerHandler dualizer, int32_t row_num, int32_t ones_num, int32_t* ones) {
    auto& state = *reinterpret_cast<DualizerState*>(dualizer);
    for (int j = 0; j < ones_num; ++j) {
        state.input_matrix.set(row_num, ones[j], true);
    }
}
