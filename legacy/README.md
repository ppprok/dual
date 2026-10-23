# legacy/

**Unmaintained. Not built. Not tested.** Code and data here are kept for reference only: none of
it is referenced by the CMake build, and it may not compile against the current `src/`. Files were
moved with `git mv`, so `git log --follow <file>` shows their full history.

The maintained part of the project stays in `src/` (`src/common`, the built part of `src/dual`,
`src/tests`, `src/pyrunc`).

## What is where

| Path | What it is | Why it is here |
|---|---|---|
| `src/logical/` | Logical-analysis classifiers (`logcls`: EC/ECmp/FT voting, boosting, logical correctors, feature selection). | Disabled in `src/CMakeLists.txt`; depends on Boost. |
| `src/dual/` | Dualization over products of partially ordered sets (`PosetsDualizationTask`, `MIGRAS`, `GraphPoset`, `IntervalLattice`, `NumberChain`, `SubsetsPoset`, `Antichain`, ...), frequent/infrequent elements enumeration (`FrequentInfrequentEnumerator.h/.cpp`), `GeneticDualizer.h/.cpp`, `JYPDualizer.h`, generic `Solver`/`ISolver`/`Callbacks`, the `dualizer` executable (`main.cpp`) and its `solvers/*.info` configs. | Not listed in `src/dual/CMakeLists.txt`; uses Boost property trees via `Configurable`. |
| `src/common/` | `Configurable` (Boost property-tree configuration), `logging_utils` (Boost.Log) and `errors.h` (Boost.Exception error tags). | Excluded from `src/common/CMakeLists.txt`; depend on Boost. |
| `src/tests/LogicalAnalysisLib.Test/` | Tests for `src/logical`. | Disabled in `src/tests/CMakeLists.txt`. |
| `src/tests/data/` | Inputs of the poset tests: `*.json` posets, `*.info` task configs, poset `*.csv` tables (`A1.csv`...`A7.csv`, `B1.csv`, `intervals*.csv`, `antichains-*`, `numberchains-*`, `pricegraph-*`). | Only read by the tests in `src/tests/DualizationLib.Test/` below. |
| `src/tests/DualizationLib.Test/` | Tests for the poset algorithms (MIGRAS, intervals, graph posets, number chains, antichains, frequent/infrequent, genetic dualizer, `PosetSelfTester`) and old MSVC `CppUnitTest` tests for AOT1, PUNC, pruning, cover collectors and file writers (AOT1, PUNC and pruning are covered by the built `AOT1PUNCTests.cpp` and `PruningTests.cpp` in `src/tests/DualizationLib.Test/`); `RuncFixedTests.cpp` (superseded by `src/tests/DualizationLib.Test/RuncFixedO2Tests.cpp`). | Not listed in the `TestDual` target; the MSVC tests also need `stdafx.h`/`CppUnitTest.h` and `../ao.test/datasets/`; `RuncFixedTests.cpp` was never built and does not compile. |
| `3rd-party/` | Reference implementations of other dualization algorithms (BEGK, BMR, DL, KS, shd31, mtminer) and Boost submodules (`boost/log`, `boost/exception`, `boost/vendor`). | All `add_subdirectory` calls are commented out in the root `CMakeLists.txt`. Each keeps its authors' own license terms. |
| `experiments/`, `configs/` | `.info` configurations, task lists and results of the logical-analysis experiments (cross-validation, leave-one-out, train/test, timing). | Inputs for the unbuilt `logcls`. |
| `mathematica/` | Mathematica notebooks (`Boosting.nb`, `LA.nb`), plots and raw result logs of those experiments. | Analysis of the logical-analysis experiments. |
| `datasets/*.int`, `test-datasets/` | Classification datasets (UCI: iris, mushrooms, nursery, zoo, ...) in the `.int` format of `src/logical`. | Only read by `src/logical` and the experiments. |
| `docs/task/` | `project_tasks_2.tex` (LaTeX, cp1251) and `project_tasks_2.pdf`: the Russian research task statement on dualization and joint enumeration of maximal/minimal independent elements. | Russian original; the English rendering is `docs/research-tasks.md`. |

## Left in place on purpose

- `src/common/csv_loader.cpp` is not compiled, but its header is used by `TestUtils` (`CSVTests.cpp`);
  it stays next to the header.
- `datasets/*.hg` (Boolean matrices / hypergraphs) stay at the top level: they are inputs for the
  maintained dualization algorithms.
