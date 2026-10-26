# Optimal minimal covers

Often not all minimal (irreducible) covers of a Boolean matrix are needed, only those that are
optimal by some functional: the shortest ones, the lightest by column weights, the ones that hit the
heaviest set of "target" rows. Finding one optimal cover is NP-hard already for the length (it is
minimum set cover / minimum hitting set), so the library does it by enumeration: the backtracking
algorithms enumerate minimal covers and a weighting callback prunes the branches that cannot contain
an optimal cover. This page explains why that pruning returns **exactly all** optimal minimal
covers (ties included), which algorithms and functionals it works for, and how much it saves
(issues [#49](https://github.com/ppprok/dual/issues/49) and, for the bounds from irreducibility
that make maximisation prune, [#53](https://github.com/ppprok/dual/issues/53)).

## Quick start

```cpp
#include <dual/optimal_covers.h>

dual::bit_matrix m = ...;                      // rows to cover
dual::Weights w = {3, 1, 4, 1, 5};             // column weights; missing columns weigh 1

dual::OptimalCoversOptions options;            // Minimize, tolerance 0, RUNCM, prune
auto lightest = dual::optimalCovers(m, w, options);
// lightest.covers: every minimal cover of the smallest total weight, columns sorted
// lightest.weight: that weight; lightest.nodes: search tree nodes visited

auto shortest = dual::optimalCovers(m, {}, options);   // empty weights: the length

options.objective = dual::Objective::Maximize;
auto widest = dual::optimalCoversByCoveredRows(m, target, rowWeights, options);
```

`optimalCovers` minimises or maximises the total column weight (empty weights give the length),
`optimalCoversByCoveredRows` the total weight of the rows of a second matrix `target` hit by the
cover. `OptimalCoversOptions::algorithm` picks RUNCM (default), RUNC, OPT, AOT1, PUNC or FK;
`prune = false` enumerates everything and filters, which is what the tests compare against;
`completionBounds = false` keeps the pruning but drops the bounds from irreducibility (#53).

The building blocks can also be used directly: a weighting callback installed as `InnerCallback`
and chained before an `OptimalCoverCollector` in `CoverCallback`:

| Weighting callback | Functional | Dynamic bound |
|---|---|---|
| `WeightBoundsPruningCallback` (`<dual/weight_bounds_pruning.h>`) | sum of column weights | `TargetToMin()` / `TargetToMax()`, `SetEpsilon(tolerance)` |
| `CoveredRowsWeighter` (`<dual/pruning.h>`) | sum of weights of hit target rows | same |
| `ColumnsWeighterCallback`, `CoveredRowWeighterCallback` (`<dual/weighters.h>`) | same two functionals | set `OptimalCoverCollector::Weighter`; the collector moves the bound |

The first two also take `EnableCompletionBounds(matrix)` (the matrix being dualized; for
`CoveredRowsWeighter` call it after `SetOptions`), which adds the bounds from irreducibility below.

`OptimalCoverCollector` (`<dual/optimal_covers.h>`) keeps every cover within `tolerance` of the best
weight seen so far and drops the ones that a later, better cover beats. `MostCoverCollector` is
different: it keeps the `N` best covers and breaks ties arbitrarily (by fewer columns, then at
random), so it does not return all optimal covers.

## Why pruning keeps exactly the optimal covers

Take minimisation of a functional `f` over the minimal covers (maximisation is symmetric). All the
search-tree algorithms here (RUNC, RUNCM, OPT, AOT1, PUNC) share three properties, which the code
and the tests confirm:

1. A node is a column set `S`, and every cover reported in its subtree is a superset of `S`
   (columns are only added on the way down).
2. Each minimal cover is reported exactly once, at a cover node.
3. `InnerCallback` sees a `NewNode` for every added column and a `BacktrackNode` for every removed
   one, in pairs, and a `pruned` new node is not expanded.

The weighting callback keeps `node.weight = f(S)` and an **admissible lower bound** `L(S)`: a number
with `L(S) <= f(C)` for every cover `C ⊇ S`. It also keeps the bound `U = best + tolerance`, where
`best` is the smallest weight of a cover reported so far (initially `U = +inf`). A new node is
pruned if `L(S) > U`, a cover if `f(C) > U`.

- **No optimal cover is lost.** Let `f*` be the optimum and `C*` any cover with `f(C*) <= f* +
  tolerance`. At any moment `best >= f*`, so `U >= f* + tolerance >= f(C*)`. Every node `S` on the
  path to `C*` has `L(S) <= f(C*) <= U`, so it is not pruned, and neither is the cover itself.
  Because the comparison is strict, this holds for ties: with `tolerance = 0`, a second cover of
  weight `f*` survives a bound of exactly `f*`.
- **Nothing else is returned.** Covers found before the optimum may pass the bound of their time;
  the collector removes them when the better cover arrives, and at the end it holds exactly the
  covers with `f(C) <= f* + tolerance`.

So the method is correct for **any functional with an admissible bound**, it does not need
monotonicity. The bounds used are:

| Functional | `L(S)` for minimisation | upper bound for maximisation |
|---|---|---|
| column weights `w`, any sign | `w(S) + ` (sum of the negative weights of the columns not in `S`) | `w(S) + ` (sum of the positive ones) |
| length | `|S|` | `|S| + ` (number of columns not in `S`) |
| weight of hit target rows, any sign | `f(S) + ` (sum of the negative weights of the target rows not hit yet) | `f(S) + ` (sum of the positive ones) |

With non-negative weights (and for the length) the lower bound is simply `f(S)`, i.e. the
functional is monotone and a branch is cut as soon as it is heavier than the best cover. A functional
that is neither additive nor monotone can be used only with a bound that is proved admissible for
it; an inadmissible bound loses optimal covers.

### Bounds from irreducibility (completion bounds, #53)

The bounds of the table above only use the weights, not the matrix, and the sum of all remaining
positive weights is almost never below the best cover: with them alone, maximisation visits as many
nodes as the full enumeration. `CompletionBounds` (`<dual/completion_bounds.h>`) adds bounds that
use the irreducibility of the covers below a node. Let `S` be a node, `R(S)` the set of
inclusion-minimal rows of the matrix (one copy of equal rows) that `S` does not hit, `u = |R(S)|`,
and `C ⊇ S` any minimal cover.

**Lemma.** Every column `j` of `C \ S` has a *private* row in `R(S)`: a row `r` with
`r ∩ C = {j}`. Different columns have different private rows. Every row of `R(S)` is hit by
`C \ S`, and pairwise disjoint rows of `R(S)` by different columns of it.

*Proof.* `C` is minimal, so `C \ {j}` is not a cover: some row `r0` has `r0 ∩ C = {j}`. Take a
minimal row `r ⊆ r0`. `C` hits `r` and `r ∩ C ⊆ r0 ∩ C = {j}`, so `r ∩ C = {j}`; as `S ⊆ C \ {j}`,
`S` does not hit `r`, so `r ∈ R(S)`. A row private to two columns would meet `C` in both. The rows
of `R(S)` are not hit by `S` and are hit by the cover `C`, hence by `C \ S`. ∎

So `C \ S` has at most `u` columns, all in the union of the rows of `R(S)`, and they can be mapped
one-to-one to rows of `R(S)` that contain them. Describe the functional by two gains per column,
`p(j) >= 0` and `q(j) <= 0`, with `sum q(j) <= f(C) - f(S) <= sum p(j)` over `j` in `C \ S` for
every such `C` (*additive* if `f(C) - f(S)` is exactly `sum (p(j) + q(j))`). Then:

| Bound | Why |
|---|---|
| `sum p <= ` the `u` largest `p` of the columns of the rows of `R(S)` | at most `u` columns, all from those rows |
| `sum p <= ` the sum over `r` in `R(S)` of `max p` in `r` | each column is at most the maximum of its private row, rows are distinct |
| `sum q <= ` the minimum over `r` in `R(S)` of `max q` in `r` (additive) | each row is hit by some column of `C \ S`, and `q <= 0` |
| `sum q >= ` the `u` smallest `q`, and `>= ` the sum over `r` of `min q` in `r` | as for `p` |
| `sum p >= ` the sum of `min p` over pairwise disjoint rows of `R(S)` (additive) | disjoint rows are hit by different columns; the rows are packed greedily, smallest first, and the single row with the largest `min p` is also tried |

`MaxGain` is the smaller of the first two plus the third, `MinGain` the fourth plus the fifth. For
the two functionals of `optimalCovers*`:

- **Column weights** `w` (the length is `w = 1`): `p = max(w, 0)`, `q = min(w, 0)`, additive. For the
  length this says that a node can reach at most `|S| + min(u, ` number of columns of the uncovered
  minimal rows`)` and at least `|S| + ` the number of pairwise disjoint uncovered rows (so at least
  one more column for a node that is not a cover).
- **Weight of hit target rows** `v`: `p(j)` is the sum of the positive `v` of the target rows that
  contain `j`, `q(j)` that of the negative ones; not additive (a target row hit by two columns
  counts once, but it is hit by at least one of them, so `f(C) - f(S)` lies between `sum q` and
  `sum p`). It is combined with the bound of #49 (the remaining positive / negative weights), so the
  smaller one wins. With non-negative row weights `q = 0` and there is no lower bound beyond `f(S)`:
  `optimalCoversByCoveredRows` then enables the completion bounds only for maximisation.

A node is pruned when `f(S) + MaxGain + slack < min` (maximisation) or `f(S) + MinGain - slack > max`
(minimisation), with the same strict comparison as above, so ties survive.

**Per algorithm.** The lemma holds for every minimal cover containing `S`, so the bounds are
admissible for any algorithm in which (a) every cover reported below a new node `S` is a minimal
cover that contains `S` and (b) the callback can follow `S`. `CompletionBounds` derives `R(S)` of a
new node of depth `d` from the state of depth `d - 1` and the added column, and keeps one state per
depth; this is exact when the current node changes only by `addColumn` (push, `NewNode`) and
`backtrack` (pop, `BacktrackNode`) of `DualizationBacktrackAlgorithmBase`, so that the node of
depth `d - 1` is the parent of the new node and every node below `S` has `S` as a prefix of its
columns. That is the case for every algorithm of the library with a search tree:

| Algorithm | How its nodes are built | Verdict |
|---|---|---|
| RUNC, RUNCM, OPT | the new node is reported before its state is computed; if the column makes `S` reducible or leaves a row that cannot be covered, the node is a leaf. Already tried columns are removed from the remaining set; children branch on the columns of one uncovered row (RUNC: the first, RUNCM: the shortest) or on all undominated ones (OPT); a cover is reported at the node of its last column | admissible: excluded columns only remove covers from the subtree, and the bounds hold for all minimal covers containing `S` |
| AOT1, all six variants (AO1, AO2, AO1K, AO2K, AO1M, AO2M) | a column `j` is added once (one `NewNode`), then the algorithm recurses once per uncovered row `i` containing `j` (the pair `(i, j)` is the real tree node), so a callback node has several subtrees, all extending `S`; `S` need not be irreducible at that moment | admissible: the lemma does not assume `S` irreducible, and if no minimal cover contains `S`, pruning loses nothing |
| PUNC | children add the columns of the top uncovered row in the given order (ties between equal weights broken at random, with an engine that advances once per expanded node); a cover is reported at the node itself when no row is left uncovered | admissible; for the other algorithms the nodes visited with the completion bounds are a subset of those without (pruned branches hold no cover that moves the dynamic bound), for PUNC not necessarily, since pruning changes the random order of later branches |
| FK, RuncFixed | no `InnerCallback` (no search tree) | nothing to prune: filtering only, as before |

The tests check the lemma-based bounds against brute force for all `2^n` column sets of 150 random
matrices (both functionals, weights of both signs), pruned = filtered full enumeration for every
algorithm, functional and direction with and without the completion bounds (data files, random
matrices, many ties, fractional weights with `tolerance = 0`), and fixed-bound pruning on RUNC,
RUNCM, OPT, PUNC and all six AOT1 variants.

**Floating point.** The bound is computed in `double` from the gains, but the weight of a cover is
the `float` sum of its terms in path order, which can differ from the exact sum by about
`k * FLT_EPSILON / 2 * (|f(S)| + M)` (`k` terms, `M` the sum of the absolute values of all terms).
A bound that is exactly tight (frequent for ties) could then prune a cover whose computed weight
equals the best one. The comparison therefore widens the bound by
`slack = 2 * (k + 1) * FLT_EPSILON * (|f(S)| + M)` (`CompletionBounds::Slack`), which covers that
error with a margin and is far below 1 for integer weights, so it costs no pruning there. Without
it, the test with fractional weights and `tolerance = 0` loses tied optima in about 30 cases.

**Cost.** Each new node updates a bit mask of the uncovered minimal rows (`m / 64` words); the
bounds are evaluated only when the dynamic bound is finite (after the first cover), cheapest first
(the `u` largest gains of all columns, then the row maxima, then the union of the rows), and stop as
soon as the answer is known. `OptimalCoversOptions::completionBounds = false` turns them off.

### Floating point

`Weight` is `float`. Two things matter:

- **Drift (fixed in #49).** The weighters used to update `node.weight` by adding the weight of a new
  column and subtracting it on backtrack. In floating point `(x + w) - w` is not always `x`, so over
  a long enumeration the same cover got slightly different weights and a dynamic bound with
  `tolerance = 0` pruned tied optimal covers (with random weights from `0.1, ..., 0.9`, RUNCM lost
  up to five of six tied optima on `10_50_7187.hg`). The weighters now save their state when a column is added
  and restore it exactly on backtrack (`WeightCheckpoints`), so the weight of a node depends only on
  its path; the regression test checks that no reported weight differs from the path-order sum.
- **Summation order.** Different covers sum their weights in different orders, so covers whose
  weights are equal as real numbers can differ in the last bit (`0.1f + 0.2f != 0.3f`). Ties are
  exact when the weights and their sums are exactly representable, e.g. integers up to `2^24`; for
  fractional weights pass a small `tolerance` (say `1e-4` times the weight scale).

## What is supported

| Algorithm | Length | Column weight (>= 0) | Column weight (any sign) | Hit target rows weight | How |
|---|---|---|---|---|---|
| RUNCM, RUNC, OPT (`Runc::RuncGlobalState`) | yes | yes | yes | yes | pruning |
| AOT1 (`AOT1::GlobalState`) | yes | yes | yes | yes | pruning |
| PUNC (`PUNC::GlobalState`) | yes | yes | yes | yes | pruning; `optimalCovers` also orders the columns from the most promising weight |
| FK (`FK::GlobalState`) | yes | yes | yes | yes | filtering only: there is no search tree and no `InnerCallback`, so nothing to prune |
| RuncFixed | yes | yes | yes | yes | filtering only: it has no callbacks (the test filters `RuncFixedDualize<bits64>`) |

"Yes" means the tests check that the result equals the full enumeration filtered by weight, for
both directions, on the data files `6_6_4` to `10_50_7187` (FK up to `10_25_480`) and on 120 random
matrices, with random integer weights chosen to create many ties; the full enumeration itself is
checked against brute force on 200 small random matrices.

Before #53 maximisation did not prune at all (the sums of the remaining weights are almost never
below the best cover); the completion bounds above fixed that, see the measurements.

## Measurements

Release preset on a shared machine (times are the best of at least three runs, but still noisy);
"nodes" are new search tree nodes (`NewNode`, pruned ones included). Three runs of the same
algorithm are compared: **#53** is the default (pruning with the completion bounds), **#49** is
`completionBounds = false` (pruning by the sums of the remaining weights only, as before #53) and
**full** is `prune = false`. Column weights are random integers 1..9; the target rows are 10
random rows with density 0.1 and weights 1..9. Reproduce with:

```bash
cmake --preset release && cmake --build --preset release
cd build/release/src/tests && ./TestDual -tc="OptimalCovers: measurements" --no-skip
```

The test prints every algorithm (RUNCM, RUNC, OPT, AOT1, PUNC); the tables show RUNCM and, in the
last column, the algorithm that visits the fewest nodes with the completion bounds.

Minimisation:

| Input | Covers | Functional | Optimal | RUNCM nodes #53 / #49 / full | RUNCM ms #53 / #49 / full | Fewest nodes (#53) |
|---|---|---|---|---|---|---|
| `15_20_275.hg` | 275 | length | 53 | 193 / 368 / 415 | 0.07 / 0.08 / 0.11 | RUNCM |
|  |  | column weight | 2 | 62 / 80 / 415 | 0.02 / 0.02 / 0.16 | PUNC 35 |
|  |  | hit rows weight | 5 | 94 / 94 / 415 | 0.02 / 0.02 / 0.13 | RUNCM |
| `10_50_7187.hg` | 7187 | length | 78 | 397 / 3767 / 8437 | 0.10 / 0.38 / 2.42 | RUNCM |
|  |  | column weight | 2 | 281 / 432 / 8437 | 0.04 / 0.04 / 2.60 | PUNC 98 |
|  |  | hit rows weight | 122 | 714 / 714 / 8437 | 0.20 / 0.20 / 3.17 | RUNCM |
| `15_35_12226.hg` | 12226 | length | 73 | 1266 / 6499 / 15203 | 0.34 / 0.91 / 4.38 | RUNCM |
|  |  | column weight | 1 | 378 / 654 / 15203 | 0.07 / 0.09 / 4.20 | PUNC 110 |
|  |  | hit rows weight | 16 | 1070 / 1070 / 15203 | 0.29 / 0.29 / 5.61 | PUNC 839 |
| random 30 x 40, density 0.2 | 180329 | length | 2 | 635 / 4680 / 236823 | 0.19 / 0.88 / 92 | RUNCM |
|  |  | column weight | 1 | 692 / 1996 / 236823 | 0.18 / 0.37 / 93 | PUNC 606 |
|  |  | hit rows weight | 40 | 3382 / 3382 / 236823 | 1.08 / 1.08 / 97 | PUNC 845 |
| random 20 x 60, density 0.1 | 997786 | length | 2 | 12282 / 102696 / 1247454 | 3.05 / 16 / 464 | PUNC 3174 |
|  |  | column weight | 3 | 2252 / 6427 / 1247454 | 0.46 / 1.09 / 527 | PUNC 594 |
|  |  | hit rows weight | 454 | 2755 / 2755 / 1247454 | 0.82 / 0.83 / 540 | RUNCM |

Maximisation:

| Input | Covers | Functional | Optimal | RUNCM nodes #53 / #49 / full | RUNCM ms #53 / #49 / full | Fewest nodes (#53) |
|---|---|---|---|---|---|---|
| `15_20_275.hg` | 275 | length | 2 | 194 / 415 / 415 | 0.08 / 0.15 / 0.13 | RUNCM |
|  |  | column weight | 1 | 178 / 415 / 415 | 0.05 / 0.14 / 0.14 | PUNC 159 |
|  |  | hit rows weight | 2 | 248 / 415 / 415 | 0.11 / 0.13 / 0.13 | RUNCM |
| `10_50_7187.hg` | 7187 | length | 419 | 1689 / 8437 / 8437 | 0.49 / 2.49 / 2.56 | RUNCM |
|  |  | column weight | 4 | 1099 / 8437 / 8437 | 0.18 / 2.28 / 2.39 | PUNC 1031 |
|  |  | hit rows weight | 1 | 2290 / 8437 / 8437 | 0.55 / 3.11 / 3.16 | PUNC 1739 |
| `15_35_12226.hg` | 12226 | length | 52 | 2335 / 15203 / 15203 | 0.69 / 4.47 / 4.50 | RUNCM |
|  |  | column weight | 4 | 754 / 15203 / 15203 | 0.15 / 4.21 / 4.43 | RUNCM |
|  |  | hit rows weight | 3 | 1402 / 15203 / 15203 | 0.36 / 5.32 / 5.42 | RUNCM |
| random 30 x 40, density 0.2 | 180329 | length | 354 | 10762 / 236823 / 236823 | 5.29 / 88 / 99 | RUNCM |
|  |  | column weight | 1 | 4976 / 236823 / 236823 | 1.35 / 88 / 96 | RUNCM |
|  |  | hit rows weight | 591 | 76417 / 236823 / 236823 | 22 / 102 / 101 | RUNCM |
| random 20 x 60, density 0.1 | 997786 | length | 92 | 1402 / 1247454 / 1247454 | 0.42 / 503 / 516 | RUNCM |
|  |  | column weight | 1 | 2063 / 1247454 / 1247454 | 0.54 / 514 / 487 | RUNCM |
|  |  | hit rows weight | 64883 | 1095178 / 1247454 / 1247454 | 297 / 556 / 570 | RUNCM |

- **Maximisation now prunes.** Before #53 it visited exactly the nodes of the full enumeration
  (#49 = full above). With the completion bounds RUNCM visits 2-20 times fewer nodes on the data
  files and 20-900 times fewer on the random matrices for the length and the column weight, and is
  faster by about as much. Over all inputs and functionals the full enumeration visits 1.1-890 times
  (RUNCM), 1.2-195 (RUNC), 1.2-330 (AOT1), 1.2-167 (PUNC) and 1.1-18 (OPT) times more nodes.
- **Minimisation prunes more** for the length (the disjoint uncovered rows give "at least one more
  column" and more): 2-9 times fewer nodes than #49 with RUNCM, up to 40 times with RUNC and PUNC;
  and 1.2-14 times fewer for the column weight (over all algorithms). For the hit rows weight with non-negative weights
  the bounds give nothing and are not enabled, so #53 = #49.
- **Where pruning cannot help.** On the 20 x 60 matrix every cover that hits all ten target rows is
  optimal (64883 of 997786 covers), so maximising the hit rows weight prunes only 12% of the nodes;
  RUNCM is still faster, but OPT, whose nodes are cheap, is up to 25% slower than without the
  completion bounds there. `completionBounds = false` turns them off for such inputs.
- The order in which covers are found matters, as before: PUNC tries the most promising columns
  first and usually needs the fewest nodes for weighted minimisation; RUNCM, which branches on the
  shortest uncovered row, profits most from the completion bounds for maximisation.

**Overhead per node.** To see the cost of the bounds without their benefit, a scratch benchmark ran
the full enumeration with a `WeightBoundsPruningCallback` whose fixed bound is finite but never
reached, so the bounds are evaluated at every new node and prune nothing (same search tree with and
without them). On `10_50_7187.hg`, `15_35_12226.hg` and the two random matrices, for RUNCM, OPT,
AOT1 and PUNC, length and column weights, both directions, the completion bounds added up to about
150 ns per node (typically 50-80 ns; 10-45% of a node, which costs 140-680 ns there). In real runs most of that is
saved: the bounds are evaluated only once a cover is known, stop as soon as the answer is known,
and every pruned node removes a subtree. In the tables above minimisation with the completion
bounds is never measurably slower than without them except on runs of a few hundredths of a
millisecond (`15_20_275.hg`), where the fixed setup cost (a few microseconds: the minimal rows and
the column masks) shows.
