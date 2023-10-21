#include "RuncExecutor.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "RUNC.h"

class DualizerState {
public:
    Runc::GlobalState state;
    bit_matrix input_matrix;

    uint32_t covers_buffer_size = 0;
    uint32_t* covers_buffer = nullptr;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> stop_ = false;

    IDualizationCallbackPtr cover_callback;

    DualizerState() {
        cover_callback = dualization_callback([this](DualizationNode& node){
            process_cover(node.columns);
            if (stop_) {
                node.stoped = true;
            }
        });
    }

    void process_cover(std::vector<int> const& cover){
        std
    }

    void set_input_one(int32_t i, int32_t j) {
        input_matrix.set(i, j);
    }

    void start_dualization() {
        state.SetMatrix(std::move(input_matrix));

        thread_ = std::make_unique<std::thread>([this]{
            state.Dualize();
        });
    }

    ~DualizerState(){
        stop_ = true;
        cv_.notify_all();
        if (thread_){
            thread_->join();
        }
    }
};

extern "C" {

DualizerHandler init_dualizer() {
    return new DualizerState();
}

void release_dualizer(DualizerHandler dualizer) {
    delete reinterpret_cast<DualizerState*>(dualizer);
}

int dualize(DualizerHandler dualizer) {
    auto p_state = reinterpret_cast<DualizerState*>(dualizer);
    p_state->dualize();
}

void set_input_row(DualizerHandler dualizer, int32_t row_num, int32_t ones_num, int32_t* ones) {
    auto& state = *reinterpret_cast<DualizerState*>(dualizer);
    for (int j = 0; j < ones_num; ++j) {
        state.input_matrix.set(row_num, ones[j]);
    }
}
}
uint32_t enumerate_covers(DualizerHandler dualizer, uint32_t buffer_size, uint32_t* covers){

    auto& state = *reinterpret_cast<DualizerState*>(dualizer);



    return 0;
}
