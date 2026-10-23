# Optimal minimal covers

Often not all minimal (irreducible) covers of a Boolean matrix are needed, only those that are
optimal by some functional: the shortest ones, the lightest by column weights, the ones that hit the
heaviest set of "target" rows. Finding one optimal cover is NP-hard already for the length (it is
minimum set cover / minimum hitting set), so the library does it by enumeration: the backtracking
algorithms enumerate minimal covers and a weighting callback prunes the branches that cannot contain
an optimal cover. This page explains why that pruning returns **exactly all** optimal minimal
covers (ties included), which algorithms and functionals it works for, and how much it saves
(issue [#49](https://github.com/ppprok/dual/issues/49)).

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
`prune = false` enumerates everything and filters, which is what the tests compare against.

The building blocks can also be used directly: a weighting callback installed as `InnerCallback`
and chained before an `OptimalCoverCollector` in `CoverCallback`:

| Weighting callback | Functional | Dynamic bound |
|---|---|---|
| `WeightBoundsPruningCallback` (`<dual/weight_bounds_pruning.h>`) | sum of column weights | `TargetToMin()` / `TargetToMax()`, `SetEpsilon(tolerance)` |
| `CoveredRowsWeighter` (`<dual/pruning.h>`) | sum of weights of hit target rows | same |
| `ColumnsWeighterCallback`, `CoveredRowWeighterCallback` (`<dual/weighters.h>`) | same two functionals | set `OptimalCoverCollector::Weighter`; the collector moves the bound |

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

**Limitation: maximisation does not prune.** The bounds above are correct but too weak for the
maximum: the sum of all remaining positive weights (or all remaining columns for the length) is
almost never below the best cover, so in the measurements below maximisation visits exactly as many
nodes as the full enumeration. Tighter bounds would use irreducibility (every column added to `S`
needs a private row among the rows not covered yet, so at most that many columns can be added) and
need the uncovered rows of the node; this is the follow-up [#53](https://github.com/ppprok/dual/issues/53). For minimisation a tighter
bound would add the cheapest column of an uncovered row.

## Measurements

Release preset, one run on a shared machine (times are noisy); "nodes" are new search tree nodes
(`NewNode`, pruned ones included), "full" is the same algorithm with `prune = false`. Column weights
are random integers 1..9; the target rows are 10 random rows with density 0.1 and weights 1..9.
Reproduce with:

```bash
cmake --preset release && cmake --build --preset release
cd build/release/src/tests && ./TestDual -tc="OptimalCovers: measurements" --no-skip
```

Minimisation (RUNCM; PUNC where it is notably better):

| Input | Covers | Functional | Optimal covers | Nodes pruned / full | ms pruned / full |
|---|---|---|---|---|---|
| `15_20_275.hg` | 275 | length | 53 | 368 / 415 | 0.2 / 0.2 |
| | | column weight | 2 | 80 / 415 (PUNC 48 / 544) | 0.04 / 0.2 |
| | | hit rows weight | 5 | 94 / 415 | 0.07 / 0.3 |
| `10_50_7187.hg` | 7187 | length | 78 | 3767 / 8437 | 0.8 / 4.6 |
| | | column weight | 2 | 432 / 8437 (PUNC 156 / 8664) | 0.2 / 3.8 |
| | | hit rows weight | 122 | 714 / 8437 | 0.4 / 5.5 |
| `15_35_12226.hg` | 12226 | length | 73 | 6499 / 15203 | 1.7 / 7.4 |
| | | column weight | 1 | 654 / 15203 (PUNC 300 / 26340) | 0.2 / 7.4 |
| | | hit rows weight | 16 | 1070 / 15203 | 0.5 / 8.9 |
| random 30 x 40, density 0.2 | 180329 | length | 2 | 4680 / 236823 | 1.5 / 137 |
| | | column weight | 1 | 1996 / 236823 | 0.4 / 92 |
| | | hit rows weight | 40 | 3382 / 236823 (PUNC 845 / 310824) | 1.4 / 112 |
| random 20 x 60, density 0.1 | 997786 | length | 2 | 102696 / 1247454 | 102 / 2277 |
| | | column weight | 3 | 6427 / 1247454 | 3.7 / 975 |
| | | hit rows weight | 454 | 2755 / 1247454 | 1.5 / 844 |

- Pruning pays off most for weighted minimisation: 4-25 times fewer nodes on the data files and
  70-450 times fewer on the random matrices; the larger the output, the larger the gain. The length
  gains less where many covers have the minimum length (about 2 times on the data files, 12 times on
  the 20 x 60 matrix), since the bound `|S|` ignores that a node that is not a cover needs at least
  one more column.
- The order in which covers are found matters: PUNC tries the cheapest columns first, finds a good
  cover early and often prunes the most; OPT (all undominated columns as branches) prunes the least.
- Maximisation: nodes pruned = nodes full for every algorithm and functional (see the limitation
  above).
