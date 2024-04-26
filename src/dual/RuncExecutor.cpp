#include "RuncExecutor.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "Runc.h"

extern "C" {

DualizerHandler init_dualizer() {
    return new Runc::RuncGlobalState();
}

void release_dualizer(DualizerHandler dualizer) {
    delete reinterpret_cast<Runc::RuncGlobalState*>(dualizer);
}

void set_input_row(DualizerHandler dualizer, int32_t row_num, int32_t ones_num, int32_t* ones) {
    auto& state = *reinterpret_cast<Runc::RuncGlobalState*>(dualizer);
    for (int j = 0; j < ones_num; ++j) {
        state.get_matrix().set(row_num, ones[j]);
    }
}
}
uint32_t enumerate_covers(DualizerHandler dualizer, uint32_t buffer_size, uint32_t* covers) {
    auto& state = *reinterpret_cast<Runc::RuncGlobalState*>(dualizer);
    return state.dualize(buffer_size, covers);
}
