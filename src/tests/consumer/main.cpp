// Consumer of the installed dual package: links only against dual::dual and includes headers by
// bare name, as a library user would. Exits with 0 when every check passes.
#include <cstdio>
#include <utility>
#include <vector>

#include "CoverCollector.h"
#include "Pruning.h"
#include "Runc.h"
#include "RuncFixed.h"

namespace {

int failures = 0;

void Check(bool ok, char const* what) {
    if (!ok) {
        std::printf("FAILED: %s\n", what);
        ++failures;
    }
}

}  // namespace

int main() {
    // Rows {0,1}, {1,2}, {0,2} of a 3-column matrix: the minimal covers are the three pairs
    std::vector<bits64> rows64 = {0b011, 0b110, 0b101};
    Check(RuncFixedDualize<bits64>(rows64).size() == 3, "RuncFixedDualize<bits64>");
    std::vector<bits32> rows32 = {0b011, 0b110, 0b101};
    Check(RuncFixedDualize<bits32>(rows32).size() == 3, "RuncFixedDualize<bits32>");
    // Disjoint pairs (X, Y), X a minimal cover of rows64 and Y of {2}: only ({0,1}, {2})
    Check(RuncFixedDualize<bits64>(rows64, {0b100}) == 1, "RuncFixedDualize<bits64> on intervals");

    // The same matrix through RUNC-M with CoveredRowsWeighter: target row {0} weighs 1, so a cover
    // weighs 1 if it contains column 0; with the upper bound 0 only the cover {1,2} remains.
    bit_matrix matrix;
    matrix.zero(3, 3);
    int ones[][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 2}, {2, 0}, {2, 2}};
    for (auto const& one : ones)
        matrix.set(one[0], one[1]);
    bit_matrix target;  // the weighter keeps views of its rows
    target.zero(1, 3);
    target.set(0, 0);

    CoveredRowsWeighter weighter;
    CoveredRowsWeighter::Options options;
    options.TargetRows = target.get_rows();
    options.DefaultWeight = 1;
    weighter.SetOptions(options);
    weighter.FixMaxBound(0);

    struct Chain : IDualizationCallback {
        CoveredRowsWeighter* weighter = nullptr;
        CoverCollector collector;
        void Call(DualizationNode& node) override {
            weighter->Call(node);
            collector.Call(node);
        }
    } chain;
    chain.weighter = &weighter;

    Runc::RuncGlobalState runc(Runc::RUNCM);
    runc.setMatrix(std::move(matrix));
    runc.InnerCallback = &weighter;
    runc.CoverCallback = &chain;
    runc.dualize();

    auto const& covers = chain.collector.GetCovers();
    Check(covers.size() == 1, "CoveredRowsWeighter keeps one cover");
    if (covers.size() == 1)
        Check(covers[0].columns.size() == 2 && covers[0].weight == 0, "CoveredRowsWeighter keeps the cover {1,2}");

    if (failures == 0)
        std::printf("dual consumer: OK\n");
    return failures == 0 ? 0 : 1;
}
