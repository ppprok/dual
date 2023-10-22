#include "RuncExecutor.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "RUNC.h"

class DualizerState {
public:
    Runc::GlobalState state;
    bit_matrix input_matrix;

    std::queue<std::vector<int>> covers_queue_;
    std::unique_ptr<std::thread> thread_;
    bool stop_ = false;

    std::mutex mutex_;
    std::condition_variable cv_;

    DualizerState() {}

    void process_cover(std::vector<int> const& cover, std::unique_lock<std::mutex>& lock) {
        covers_queue_.push(cover);
        if (covers_queue_.size() >= 128 || stop_) {
            cv_.notify_all();
            cv_.wait(lock, [&] { return covers_queue_.empty() || stop_; });
        }
    }

    void set_input_one(int32_t i, int32_t j) {
        input_matrix.set(i, j);
    }

    void start_dualization() {
        state.SetMatrix(std::move(input_matrix));

        thread_ = std::make_unique<std::thread>([this] {
            std::unique_lock lock(mutex_);

            auto cover_callback = dualization_callback([this, &lock](DualizationNode& node) {
                process_cover(node.columns, lock);
                if (stop_) {
                    node.stoped = true;
                }
            });
            state.CoverCallback = cover_callback.get();
            state.Dualize();
            covers_queue_.emplace();
            cv_.notify_one();
        });
    }

    ~DualizerState() {
        {
            std::lock_guard lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        if (thread_) {
            thread_->join();
        }
    }

    uint32_t enumerate_covers(uint32_t buffer_size, uint32_t* buffer) {
        auto width = input_matrix.width();

        uint32_t count = 0;

        std::unique_lock lock(mutex_);

        if (!thread_) {
            start_dualization();
        }

        if (stop_) {
            return 0;
        }

        while (buffer_size >= width + 1) {

            if (covers_queue_.empty())
                cv_.notify_one();
            cv_.wait(lock, [&] { return !covers_queue_.empty() || stop_; });
            if (stop_)
                return count;

            auto cover = covers_queue_.front();
            covers_queue_.pop();
            if (cover.empty()) {
                stop_ = true;
                return count;
            }

            buffer[0] = cover.size();
            ++buffer;
            --buffer_size;

            std::copy(cover.begin(), cover.end(), buffer);
            buffer += cover.size();
            buffer_size -= cover.size();
            ++count;
        }

        return count;
    }
};

extern "C" {

DualizerHandler init_dualizer() {
    return new DualizerState();
}

void release_dualizer(DualizerHandler dualizer) {
    delete reinterpret_cast<DualizerState*>(dualizer);
}

void set_input_row(DualizerHandler dualizer, int32_t row_num, int32_t ones_num, int32_t* ones) {
    auto& state = *reinterpret_cast<DualizerState*>(dualizer);
    for (int j = 0; j < ones_num; ++j) {
        state.input_matrix.set(row_num, ones[j]);
    }
}
}
uint32_t enumerate_covers(DualizerHandler dualizer, uint32_t buffer_size, uint32_t* covers) {

    auto& state = *reinterpret_cast<DualizerState*>(dualizer);
    return state.enumerate_covers(buffer_size, covers);
}
