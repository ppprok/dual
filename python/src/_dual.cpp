// pybind11 extension module `dual._dual`: low-level bindings of the dualization algorithms.
//
// The user-facing API (`dual.enumerate_covers` and friends) is the Python package in
// `python/dual/__init__.py`; this module exposes the class `Dualizer`, the duality test `is_dual`
// and a debug counter.
//
// All includes of the C++ library and all references to its API are kept in this file. The library
// is used through its public headers `<dual/...>`; its names are in namespace `dual` (#12).

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <atomic>
#include <climits>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <dual/aot1.h>
#include <dual/bit_matrix.h>
#include <dual/dualization_backtrack_algorithm_base.h>
#include <dual/fk.h>
#include <dual/punc.h>
#include <dual/runc.h>

namespace py = pybind11;

namespace {

// ---------------------------------------------------------------------------------------------
// Library API used by the bindings (the part to adapt when the public headers change)

using namespace dual;

using Algorithm = DualizationBacktrackAlgorithmBase;
using Matrix = bit_matrix;
using Cover = Columns;  // std::vector<int>

enum class Family {
    Runc,
    AOT1,
    PUNC,
    FK
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
        {"fk", Family::FK, 0},
};

std::unique_ptr<Algorithm> makeAlgorithm(AlgorithmInfo const& info) {
    switch (info.family) {
        case Family::Runc:
            return std::make_unique<Runc::RuncGlobalState>(static_cast<Runc::AlgorithmType>(info.type));
        case Family::AOT1:
            return std::make_unique<AOT1::GlobalState>(static_cast<AOT1::AlgorithmType>(info.type));
        case Family::PUNC:
            return std::make_unique<PUNC::GlobalState>();
        case Family::FK:
            return std::make_unique<FK::GlobalState>();
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

// Fredman-Khachiyan duality test: nullopt if dual, otherwise the kind of the witness and its columns.
std::optional<std::pair<std::string, Cover>> dualityTest(Matrix const& h, Matrix const& g) {
    FK::DualityResult result = FK::isDual(h, g);
    switch (result.result) {
        case FK::Duality::Dual:
            return std::nullopt;
        case FK::Duality::NewTransversal:
            return std::make_pair(std::string("new_transversal"), std::move(result.witness));
        case FK::Duality::NotTransversal:
            return std::make_pair(std::string("not_transversal"), std::move(result.witness));
    }
    throw std::logic_error("unknown duality test result");
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
// portion. RUNC-family algorithms resume the search between portions; FK finds one cover more per
// call of FK::GlobalState::next(); AOT1 and PUNC have no resumable mode, so their covers are
// computed in full on the first portion and then handed out.
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
        return info_.family == Family::Runc || info_.family == Family::FK;
    }

    Matrix const& matrix() const {
        return alg_->get_matrix();
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
        if (info_.family == Family::Runc)
            return runc().isDualizeFinished();
        if (info_.family == Family::FK)
            return fk().isDualizeFinished();
        return computed_ && next_ == collected_.size();
    }

    std::vector<Cover> next_batch(size_t max_count) {
        Busy busy(*this);
        std::vector<Cover> covers;
        {
            py::gil_scoped_release release;
            if (info_.family == Family::Runc) {
                covers = runc().dualize(max_count);
            } else if (info_.family == Family::FK) {
                for (size_t k = 0; k < max_count && fk().next(); ++k)
                    covers.push_back(fk().getLastCover());
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

    std::optional<Cover> next_cover() {
        std::vector<Cover> covers = next_batch(1);
        if (covers.empty())
            return std::nullopt;
        return std::move(covers.front());
    }

    // All covers in one run of the algorithm (for FK much faster than portions: FK::GlobalState::dualize()
    // does not repeat the duality test from the root for each cover); restarts the enumeration.
    std::vector<Cover> all_covers() {
        Busy busy(*this);
        std::vector<Cover> covers;
        {
            py::gil_scoped_release release;
            covers = collectAll();
            for (auto& cover : covers)
                std::sort(cover.begin(), cover.end());
        }
        resetEnumeration();
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

    // Duality test of the matrices of two dualizers (the same object may be passed twice).
    static std::optional<std::pair<std::string, Cover>> isDual(Dualizer& h, Dualizer& g) {
        Busy busyH(h);
        std::optional<Busy> busyG;
        if (&g != &h)
            busyG.emplace(g);
        py::gil_scoped_release release;
        return dualityTest(h.matrix(), g.matrix());
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

    FK::GlobalState& fk() const {
        return static_cast<FK::GlobalState&>(*alg_);
    }

    void setMatrix(Matrix matrix) {
        alg_->setMatrix(std::move(matrix));
        resetEnumeration();
    }

    void resetEnumeration() {
        if (info_.family == Family::Runc)
            runc().resetDualize();
        else if (info_.family == Family::FK)
            fk().resetDualize();
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
memory does not grow with the number of covers. FK ('fk', Khachiyan's incremental dualization)
is resumable too: each next cover is found by a Fredman-Khachiyan duality test of the covers
found so far, which it keeps. The AOT1 ('ao1', ..., 'ao2m') and PUNC ('punc') algorithms compute
all covers on the first portion. Covers are lists of 0-based column indices
in increasing order. Setting a new matrix or calling reset() restarts the enumeration.
)doc")
            .def(py::init<std::string const&>(),
                 py::arg("algorithm") = "runcm",
                 "Create a dualizer for the given algorithm with an empty matrix (0 rows).")
            .def_property_readonly("algorithm", &Dualizer::algorithm, "Name of the algorithm.")
            .def_property_readonly("resumable",
                                   &Dualizer::resumable,
                                   "True if the search is resumed between portions (RUNC family and FK).")
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
            .def("next_cover",
                 &Dualizer::next_cover,
                 "Return the next cover, or None once all covers have been returned.")
            .def("all_covers",
                 &Dualizer::all_covers,
                 "Return all covers computed in one run (faster than portions for 'fk'); restarts the enumeration.")
            .def("count",
                 &Dualizer::count,
                 "Count all minimal covers without returning them; restarts the enumeration.")
            .def("reset", &Dualizer::reset, "Restart the enumeration from the first cover.");

    m.def("is_dual",
          &Dualizer::isDual,
          py::arg("h"),
          py::arg("g"),
          R"doc(
Fredman-Khachiyan duality test (algorithm B) of the matrices of two dualizers.

Returns None if the minimal rows of g are exactly the minimal covers of h, otherwise a pair
(kind, witness): kind 'new_transversal' means the witness (a sorted list of columns) covers h and
contains no row of g; 'not_transversal' means the witness contains a row of g and its complement
contains a row of h, so that row of g is not a cover of h.
)doc");
}
