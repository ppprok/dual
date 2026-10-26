// Consumer of the installed dual package: links only against dual::dual and includes the public
// headers as <dual/...>, as a library user would. Exits with 0 when every check passes.
#include <cstdio>
#include <utility>
#include <vector>

#include <dual/completion_bounds.h>
#include <dual/cover_collector.h>
#include <dual/fk.h>
#include <dual/optimal_covers.h>
#include <dual/pruning.h>
#include <dual/runc.h>
#include <dual/runc_fixed.h>

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
    std::vector<dual::bits64> rows64 = {0b011, 0b110, 0b101};
    Check(dual::RuncFixedDualize<dual::bits64>(rows64).size() == 3, "RuncFixedDualize<dual::bits64>");
    std::vector<dual::bits32> rows32 = {0b011, 0b110, 0b101};
    Check(dual::RuncFixedDualize<dual::bits32>(rows32).size() == 3, "RuncFixedDualize<dual::bits32>");
    // Disjoint pairs (X, Y), X a minimal cover of rows64 and Y of {2}: only ({0,1}, {2})
    Check(dual::RuncFixedDualize<dual::bits64>(rows64, {0b100}) == 1, "RuncFixedDualize<dual::bits64> on intervals");

    // The same matrix through RUNC-M with CoveredRowsWeighter: target row {0} weighs 1, so a cover
    // weighs 1 if it contains column 0; with the upper bound 0 only the cover {1,2} remains.
    dual::bit_matrix matrix;
    matrix.zero(3, 3);
    int ones[][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 2}, {2, 0}, {2, 2}};
    for (auto const& one : ones)
        matrix.set(one[0], one[1]);
    dual::bit_matrix target;  // the weighter keeps views of its rows
    target.zero(1, 3);
    target.set(0, 0);

    dual::CoveredRowsWeighter weighter;
    dual::CoveredRowsWeighter::Options options;
    options.TargetRows = target.get_rows();
    options.DefaultWeight = 1;
    weighter.SetOptions(options);
    weighter.FixMaxBound(0);

    struct Chain : dual::IDualizationCallback {
        dual::CoveredRowsWeighter* weighter = nullptr;
        dual::CoverCollector collector;
        void Call(dual::DualizationNode& node) override {
            weighter->Call(node);
            collector.Call(node);
        }
    } chain;
    chain.weighter = &weighter;

    dual::Runc::RuncGlobalState runc(dual::Runc::RUNCM);
    runc.setMatrix(std::move(matrix));
    runc.InnerCallback = &weighter;
    runc.CoverCallback = &chain;
    runc.dualize();

    auto const& covers = chain.collector.GetCovers();
    Check(covers.size() == 1, "CoveredRowsWeighter keeps one cover");
    if (covers.size() == 1)
        Check(covers[0].columns.size() == 2 && covers[0].weight == 0, "CoveredRowsWeighter keeps the cover {1,2}");

    // Khachiyan's incremental dualization of the triangle {0,1}, {1,2}, {0,2}: the three pairs, one by
    // one, and the duality test of the triangle with itself (it is self-dual)
    dual::bit_matrix triangle(3, 3);
    int edges[][2] = {{0, 1}, {1, 2}, {0, 2}};
    for (int i = 0; i < 3; ++i)
        for (int j : edges[i])
            triangle.set(i, j);
    Check(dual::FK::isDual(triangle, triangle).dual(), "FK::isDual on the self-dual triangle");
    dual::FK::GlobalState fk;
    fk.setMatrix(std::move(triangle));
    int found = 0;
    while (fk.next())
        ++found;
    Check(found == 3 && fk.getCovers().height() == 3, "FK::GlobalState::next finds three covers");

    // Optimal covers of the triangle with column weights 1, 2, 3: the lightest is {0,1} (weight 3)
    dual::bit_matrix triangle2(3, 3);
    for (int i = 0; i < 3; ++i)
        for (int j : edges[i])
            triangle2.set(i, j);
    auto best = dual::optimalCovers(triangle2, {1, 2, 3});
    Check(best.covers.size() == 1 && best.weight == 3 && best.covers[0].columns == dual::Columns{0, 1},
          "optimalCovers finds the lightest cover");
    dual::OptimalCoversOptions heaviest;  // pruned by the completion bounds (#53)
    heaviest.objective = dual::Objective::Maximize;
    best = dual::optimalCovers(triangle2, {1, 2, 3}, heaviest);
    Check(best.covers.size() == 1 && best.weight == 5 && best.covers[0].columns == dual::Columns{1, 2},
          "optimalCovers finds the heaviest cover");
    dual::CompletionBounds bounds;
    bounds.SetMatrix(triangle2);
    Check(bounds.HasMatrix() && !bounds.Enabled(), "CompletionBounds needs gains");

    if (failures == 0)
        std::printf("dual consumer: OK\n");
    return failures == 0 ? 0 : 1;
}
