// pybind11 extension module `dual._dual`: low-level bindings of the dualization algorithms.
//
// The user-facing API (`dual.enumerate_covers` and friends) is the Python package in
// `python/dual/__init__.py`; this module exposes one class, `Dualizer`, and a debug counter.
//
// All includes of the C++ library and all references to its API are kept in this file, so that
// changes of the public headers (#12: `<dual/...>` paths, namespace `dual`) are a local edit.

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <atomic>
#include <climits>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "AOT1.h"
#include "DualizationBacktrackAlgorithmBase.h"
#include "PUNC.h"
#include "Runc.h"
#include "bit_matrix.h"

namespace py = pybind11;

namespace {

// ---------------------------------------------------------------------------------------------
// Library API used by the bindings (the part to adapt when the public headers change)

using Algorithm = DualizationBacktrackAlgorithmBase;
using Matrix = bit_matrix;
using Cover = Columns;  // std::vector<int>

enum class Family {
    Runc,
    AOT1,
    PUNC
};

struct AlgorithmInfo {
    const char* name;
    Family family;
    int type;
};

const AlgorithmInfo kAlgorithms[] = {
        {"runcm", Family::Runc, Runc::RUNCM},
        {"runc", Family::Runc, Runc::RUNC},
        {"opt", Family::Runc, Runc::OPT},
        {"ao1", Family::AOT1, AOT1::AO1},
        {"ao2", Family::AOT1, AOT1::AO2},
        {"ao1k", Family::AOT1, AOT1::AO1K},
        {"ao2k", Family::AOT1, AOT1::AO2K},
        {"ao1m", Family::AOT1, AOT1::AO1M},
        {"ao2m", Family::AOT1, AOT1::AO2M},
        {"punc", Family::PUNC, 0},
};

std::unique_ptr<Algorithm> makeAlgorithm(AlgorithmInfo const& info) {
    switch (info.family) {
        case Family::Runc:
            return std::make_unique<Runc::RuncGlobalState>(static_cast<Runc::AlgorithmType>(info.type));
        case Family::AOT1:
            return std::make_unique<AOT1::GlobalState>(static_cast<AOT1::AlgorithmType>(info.type));
        case Family::PUNC:
            return std::make_unique<PUNC::GlobalState>();
    }
    throw std::logic_error("unknown algorithm family");
}

// Matrix with m zero rows and n columns; throws std::length_error if it does not fit into the
// int-sized block counters of bit_matrix.
Matrix zeroMatrix(size_t m, size_t n) {
    if (m > INT_MAX || n >= INT_MAX
        || static_cast<uint64_t>(m) * static_cast<uint64_t>(bit_chunk::block_for_bits(static_cast<int>(n))) > INT_MAX)
        throw std::length_error("the matrix is too large");
    Matrix matrix;
    matrix.zero(static_cast<int>(m), static_cast<int>(n));
    return matrix;
}

void setBit(Matrix& matrix, size_t i, size_t j) {
    matrix.get_row(static_cast<int>(i)).set(static_cast<int>(j));
}

// ---------------------------------------------------------------------------------------------

std::atomic<long> g_live_dualizers{0};

const AlgorithmInfo& findAlgorithm(std::string const& name) {
    for (auto const& info : kAlgorithms)
        if (name == info.name)
            return info;
    std::string names;
    for (auto const& info : kAlgorithms)
        names += std::string(names.empty() ? "" : ", ") + "'" + info.name + "'";
    throw py::value_error("unknown algorithm '" + name + "'; expected one of " + names);
}

// Holds a dualization algorithm and its input matrix, and returns the minimal covers portion by
// portion. RUNC-family algorithms resume the search between portions; AOT1 and PUNC have no
// resumable mode, so their covers are computed in full on the first portion and then handed out.
//
// Long calls release the GIL. A Dualizer used by another thread at the same time raises
// RuntimeError instead of blocking or corrupting its state.
class Dualizer {
public:
    explicit Dualizer(std::string const& algorithm) : info_(findAlgorithm(algorithm)), alg_(makeAlgorithm(info_)) {
        ++g_live_dualizers;
    }

    ~Dualizer() {
        --g_live_dualizers;
    }

    Dualizer(Dualizer const&) = delete;
    Dualizer& operator=(Dualizer const&) = delete;

    std::string algorithm() const {
        return info_.name;
    }

    bool resumable() const {
        return info_.family == Family::Runc;
    }

    int n_rows() const {
        return alg_->get_matrix().height();
    }

    int n_cols() const {
        return alg_->get_matrix().width();
    }

    void set_rows(std::vector<std::vector<long long>> const& rows) {
        Busy busy(*this);
        long long n = 0;
        for (auto const& row : rows)
            for (auto j : row) {
                if (j < 0 || j >= INT_MAX)
                    throw py::value_error("column index " + std::to_string(j) + " is out of range [0, 2**31 - 1)");
                n = std::max(n, j + 1);
            }
        Matrix matrix = zeroMatrix(rows.size(), static_cast<size_t>(n));
        for (size_t i = 0; i < rows.size(); ++i)
            for (auto j : rows[i])
                setBit(matrix, i, static_cast<size_t>(j));
        setMatrix(std::move(matrix));
    }

    void set_dense(py::buffer buffer) {
        Busy busy(*this);
        py::buffer_info info = buffer.request();
        if (info.ndim != 2)
            throw py::value_error("a dense matrix must be 2-dimensional, got " + std::to_string(info.ndim)
                                  + " dimensions");
        if (info.itemsize != 1)
            throw py::value_error("a dense matrix must have 1-byte items (bool or uint8)");
        py::ssize_t m = info.shape[0], n = info.shape[1];
        Matrix matrix = zeroMatrix(static_cast<size_t>(m), static_cast<size_t>(n));
        auto const* data = static_cast<const char*>(info.ptr);
        for (py::ssize_t i = 0; i < m; ++i)
            for (py::ssize_t j = 0; j < n; ++j)
                if (data[i * info.strides[0] + j * info.strides[1]] != 0)
                    setBit(matrix, static_cast<size_t>(i), static_cast<size_t>(j));
        setMatrix(std::move(matrix));
    }

    void load_hg(std::string const& path) {
        Busy busy(*this);
        py::gil_scoped_release release;
        alg_->loadMatrix(path);
        resetEnumeration();
    }

    void reset() {
        Busy busy(*this);
        resetEnumeration();
    }

    bool finished() const {
        if (resumable())
            return runc().isDualizeFinished();
        return computed_ && next_ == collected_.size();
    }

    std::vector<Cover> next_batch(size_t max_count) {
        Busy busy(*this);
        std::vector<Cover> covers;
        {
            py::gil_scoped_release release;
            if (resumable()) {
                covers = runc().dualize(max_count);
            } else {
                if (!computed_) {
                    collected_ = collectAll();
                    computed_ = true;
                    next_ = 0;
                }
                size_t count = std::min(max_count, collected_.size() - next_);
                covers.reserve(count);
                for (size_t k = 0; k < count; ++k)
                    covers.push_back(std::move(collected_[next_ + k]));
                next_ += count;
                if (next_ == collected_.size()) {  // all handed out: free the memory, stay finished
                    collected_ = std::vector<Cover>();
                    next_ = 0;
                }
            }
            for (auto& cover : covers)
                std::sort(cover.begin(), cover.end());
        }
        return covers;
    }

    uint64_t count() {
        Busy busy(*this);
        uint64_t total = 0;
        {
            py::gil_scoped_release release;
            auto counter = dualization_callback([&total](DualizationNode&) { ++total; });
            runWithCallback(counter.get());
        }
        resetEnumeration();
        return total;
    }

private:
    // Marks the object as in use for the duration of a call; the GIL is held while it is created.
    class Busy {
    public:
        explicit Busy(Dualizer& d) : lock_(d.mutex_, std::try_to_lock) {
            if (!lock_.owns_lock())
                throw std::runtime_error("this Dualizer is being used by another thread");
        }

    private:
        std::unique_lock<std::mutex> lock_;
    };

    Runc::RuncGlobalState& runc() const {
        return static_cast<Runc::RuncGlobalState&>(*alg_);
    }

    void setMatrix(Matrix matrix) {
        alg_->setMatrix(std::move(matrix));
        resetEnumeration();
    }

    void resetEnumeration() {
        if (resumable())
            runc().resetDualize();
        collected_ = std::vector<Cover>();
        computed_ = false;
        next_ = 0;
    }

    // Full enumeration from scratch, reporting covers to `callback`; restores CoverCallback.
    void runWithCallback(IDualizationCallback* callback) {
        auto* saved = alg_->CoverCallback;
        alg_->CoverCallback = callback;
        try {
            alg_->dualize();
        }
        catch (...) {
            alg_->CoverCallback = saved;
            throw;
        }
        alg_->CoverCallback = saved;
    }

    std::vector<Cover> collectAll() {
        std::vector<Cover> covers;
        auto collector = dualization_callback([&covers](DualizationNode& node) { covers.push_back(node.columns); });
        runWithCallback(collector.get());
        return covers;
    }

    AlgorithmInfo const& info_;
    std::unique_ptr<Algorithm> alg_;
    std::mutex mutex_;

    // Non-resumable algorithms: all covers, computed on the first portion
    std::vector<Cover> collected_;
    size_t next_ = 0;
    bool computed_ = false;
};

}  // namespace

PYBIND11_MODULE(_dual, m) {
    m.doc() = "Low-level bindings of the dual C++ library; use the functions of the `dual` package instead.";

    py::list names;
    for (auto const& info : kAlgorithms)
        names.append(info.name);
    m.attr("ALGORITHMS") = py::tuple(names);

    m.def(
            "_live_dualizers",
            []() { return g_live_dualizers.load(); },
            "Number of Dualizer objects alive (for leak tests).");

    py::class_<Dualizer>(m, "Dualizer", R"doc(
Enumerator of the minimal covers of a Boolean matrix, returning them portion by portion.

The RUNC-family algorithms ('runcm', 'runc', 'opt') resume the search between portions, so
memory does not grow with the number of covers. The AOT1 ('ao1', ..., 'ao2m') and PUNC ('punc')
algorithms compute all covers on the first portion. Covers are lists of 0-based column indices
in increasing order. Setting a new matrix or calling reset() restarts the enumeration.
)doc")
            .def(py::init<std::string const&>(),
                 py::arg("algorithm") = "runcm",
                 "Create a dualizer for the given algorithm with an empty matrix (0 rows).")
            .def_property_readonly("algorithm", &Dualizer::algorithm, "Name of the algorithm.")
            .def_property_readonly("resumable",
                                   &Dualizer::resumable,
                                   "True if the search is resumed between portions (RUNC family).")
            .def_property_readonly("n_rows", &Dualizer::n_rows, "Number of rows of the matrix.")
            .def_property_readonly("n_cols", &Dualizer::n_cols, "Number of columns of the matrix.")
            .def_property_readonly("finished",
                                   &Dualizer::finished,
                                   "True once the started enumeration has returned all covers.")
            .def("set_rows",
                 &Dualizer::set_rows,
                 py::arg("rows"),
                 "Set the matrix from rows given as sequences of the column indices of their ones.")
            .def("set_dense",
                 &Dualizer::set_dense,
                 py::arg("matrix"),
                 "Set the matrix from a 2-D buffer with 1-byte items (bool or uint8); nonzero means one.")
            .def("load_hg", &Dualizer::load_hg, py::arg("path"), "Load the matrix from a .hg file.")
            .def("next_batch",
                 &Dualizer::next_batch,
                 py::arg("max_count"),
                 "Return up to max_count next covers; an empty list (for max_count > 0) means the end.")
            .def("count",
                 &Dualizer::count,
                 "Count all minimal covers without returning them; restarts the enumeration.")
            .def("reset", &Dualizer::reset, "Restart the enumeration from the first cover.");
}
