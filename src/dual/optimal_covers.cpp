#include <dual/optimal_covers.h>

#include <algorithm>
#include <functional>
#include <memory>

#include <dual/aot1.h>
#include <dual/fk.h>
#include <dual/pruning.h>
#include <dual/punc.h>
#include <dual/runc.h>
#include <dual/weight_bounds_pruning.h>
#include <dual/weighters.h>

namespace dual {

OptimalCoverCollector::OptimalCoverCollector(Objective objective, Weight tolerance) :
        objective_(objective), tolerance_(tolerance < 0 ? 0 : tolerance) {}

bool OptimalCoverCollector::better(Weight a, Weight b) const {
    return objective_ == Objective::Minimize ? a < b : a > b;
}

bool OptimalCoverCollector::withinTolerance(Weight w) const {
    return objective_ == Objective::Minimize ? w <= best_ + tolerance_ : w >= best_ - tolerance_;
}

void OptimalCoverCollector::Call(DualizationNode& node) {
    if (!node.IsCover())
        return;

    Weight w = node.weight;
    if (!hasBest_ || better(w, best_)) {
        best_ = w;
        hasBest_ = true;
        covers_.erase(std::remove_if(covers_.begin(),
                                     covers_.end(),
                                     [this](WeightedCover const& c) { return !withinTolerance(c.weight); }),
                      covers_.end());
        if (Weighter) {
            if (objective_ == Objective::Minimize)
                Weighter->SetMaxWeight(best_ + tolerance_);
            else
                Weighter->SetMinWeight(best_ - tolerance_);
        }
    } else if (!withinTolerance(w)) {
        return;
    }

    covers_.emplace_back(node.columns, w);
}

WeightedCovers const& OptimalCoverCollector::GetCovers() const {
    return covers_;
}

Weight OptimalCoverCollector::GetBestWeight() const {
    return best_;
}

bool OptimalCoverCollector::HasCovers() const {
    return hasBest_;
}

void OptimalCoverCollector::Reset() {
    covers_.clear();
    hasBest_ = false;
    best_ = 0;
}

namespace {

bit_matrix copyMatrix(bit_matrix const& matrix) {
    bit_matrix copy;
    copy.zero(matrix.height(), matrix.width());
    for (int i = 0; i < matrix.height(); ++i)
        copy.assign_row(i, matrix.get_row(i));
    return copy;
}

// Counts new nodes and forwards them to the weighter
struct CountingCallback : IDualizationCallback {
    IDualizationCallback* inner = nullptr;
    long long nodes = 0;

    void Call(DualizationNode& node) override {
        if (node.type == NodeType::NewNode)
            ++nodes;
        if (inner)
            inner->Call(node);
    }
};

// Weighs (FK) or prunes a cover, then hands it to the collector
struct CoverChain : IDualizationCallback {
    IDualizationCallback* pruner = nullptr;
    std::function<Weight(Columns const&)> weigh;  // for algorithms without inner nodes
    OptimalCoverCollector* collector = nullptr;
    long long covers = 0;

    void Call(DualizationNode& node) override {
        ++covers;
        if (weigh)
            node.weight = weigh(node.columns);
        if (pruner)
            pruner->Call(node);
        if (!node.stoped)
            collector->Call(node);
    }
};

// Algorithm with a search tree; PUNC tries the columns in increasing order of colsOrder
std::unique_ptr<DualizationBacktrackAlgorithmBase> makeAlgorithm(CoverAlgorithm algorithm, Weights colsOrder) {
    switch (algorithm) {
        case CoverAlgorithm::RUNC:
            return std::make_unique<Runc::RuncGlobalState>(Runc::RUNC);
        case CoverAlgorithm::OPT:
            return std::make_unique<Runc::RuncGlobalState>(Runc::OPT);
        case CoverAlgorithm::AOT1:
            return std::make_unique<AOT1::GlobalState>(AOT1::AO1M);
        case CoverAlgorithm::PUNC: {
            auto punc = std::make_unique<PUNC::GlobalState>();
            punc->SetColsOrder(std::move(colsOrder));
            return punc;
        }
        case CoverAlgorithm::FK:
            return std::make_unique<FK::GlobalState>();
        case CoverAlgorithm::RUNCM:
            break;
    }
    return std::make_unique<Runc::RuncGlobalState>(Runc::RUNCM);
}

// Runs the algorithm with the given weighting/pruning callback (null for FK) and returns the sorted
// optimal covers
OptimalCovers run(bit_matrix const& matrix,
                  OptimalCoversOptions const& options,
                  Weights colsOrder,
                  IDualizationCallback* pruner,
                  std::function<Weight(Columns const&)> weigh) {
    if (options.objective == Objective::Maximize)
        for (auto& w : colsOrder)
            w = -w;

    auto algorithm = makeAlgorithm(options.algorithm, std::move(colsOrder));
    algorithm->setMatrix(copyMatrix(matrix));

    OptimalCoverCollector collector(options.objective, options.tolerance);
    CountingCallback inner;
    CoverChain cover;
    cover.collector = &collector;
    if (options.algorithm == CoverAlgorithm::FK) {
        cover.weigh = std::move(weigh);
    } else {
        inner.inner = pruner;
        cover.pruner = pruner;
        algorithm->InnerCallback = &inner;
    }
    algorithm->CoverCallback = &cover;
    algorithm->dualize();

    OptimalCovers result;
    result.nodes = inner.nodes;
    result.coversVisited = cover.covers;
    result.weight = collector.HasCovers() ? collector.GetBestWeight() : 0;
    result.covers = collector.GetCovers();
    for (auto& c : result.covers)
        std::sort(c.columns.begin(), c.columns.end());
    std::sort(result.covers.begin(), result.covers.end(), [](WeightedCover const& a, WeightedCover const& b) {
        return a.columns < b.columns;
    });
    return result;
}

}  // namespace

OptimalCovers optimalCovers(bit_matrix const& matrix,
                            Weights const& columnWeights,
                            OptimalCoversOptions const& options) {
    int n = matrix.width();
    Weights weights(std::max<int>(n, (int) columnWeights.size()), 1);
    std::copy(columnWeights.begin(), columnWeights.end(), weights.begin());

    WeightBoundsPruningCallback pruner;
    pruner.SetWeights(weights);  // every column has an explicit weight
    pruner.SetDefaultWeight(0);
    if (options.prune) {
        pruner.SetEpsilon(options.tolerance < 0 ? 0 : options.tolerance);
        if (options.objective == Objective::Minimize)
            pruner.TargetToMin();
        else
            pruner.TargetToMax();
    }

    auto weigh = [&weights](Columns const& columns) {
        Weight w = 0;
        for (int j : columns)
            w += j < (int) weights.size() ? weights[j] : 1;
        return w;
    };
    return run(matrix, options, weights, &pruner, weigh);
}

OptimalCovers optimalCoversByCoveredRows(bit_matrix const& matrix,
                                         bit_matrix const& targetRows,
                                         Weights const& rowWeights,
                                         OptimalCoversOptions const& options) {
    // Copy the target rows to a width that covers every column of the matrix
    int m = targetRows.height();
    int n = std::max(matrix.width(), targetRows.width());
    bit_matrix target;
    target.zero(m, n);
    for (int i = 0; i < m; ++i) {
        auto const& row = targetRows.get_row(i);
        for (int j = row.find_first(); j >= 0; j = row.find_next(j))
            target.set(i, j);
    }

    Weights weights(std::max<int>(m, (int) rowWeights.size()), 1);
    std::copy(rowWeights.begin(), rowWeights.end(), weights.begin());
    weights.resize(m);

    CoveredRowsWeighter pruner;
    CoveredRowsWeighter::Options weighterOptions;
    weighterOptions.TargetRows = target.get_rows();
    weighterOptions.RowsWeights = weights;
    pruner.SetOptions(weighterOptions);
    if (options.prune) {
        pruner.SetEpsilon(options.tolerance < 0 ? 0 : options.tolerance);
        if (options.objective == Objective::Minimize)
            pruner.TargetToMin();
        else
            pruner.TargetToMax();
    }

    // Column order for PUNC: the total weight of the target rows each column covers
    Weights colsOrder(n, 0);
    for (int i = 0; i < m; ++i) {
        auto const& row = target.get_row(i);
        for (int j = row.find_first(); j >= 0; j = row.find_next(j))
            colsOrder[j] += weights[i];
    }

    auto weigh = [&target, &weights, m](Columns const& columns) {
        Weight w = 0;
        for (int i = 0; i < m; ++i) {
            auto const& row = target.get_row(i);
            for (int j : columns)
                if (row.get(j)) {
                    w += weights[i];
                    break;
                }
        }
        return w;
    };
    return run(matrix, options, std::move(colsOrder), &pruner, weigh);
}

}  // namespace dual
