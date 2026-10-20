# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

C++17 research code for **dualization** algorithms: enumerating all minimal covers of a Boolean matrix (equivalently, minimal transversals of a hypergraph), plus generalizations over products of partially ordered sets. The actively maintained piece is the **RUNC** family of algorithms, exposed to Python through a C ABI shared library. Literature behind the algorithms is listed in `docs/references.md` (the owner's own paper and dissertation are in `docs/references/`). License: MIT (`LICENSE`).

## Build and test

```bash
cmake -S . -B build            # needs spdlog installed (find_package REQUIRED); fetches doctest from GitHub (network)
cmake --build build -j

# Tests open data files by relative path ("data/6_6_4.hg"), so run them from build/src/tests,
# where the InstallTestUtilsData target copies src/tests/data/.
cd build/src/tests
./TestDual                     # RUNC tests
./TestUtils                    # common/ utilities tests
./TestDual -tc="DualizeRUNCTest1"   # single doctest case (wildcards allowed: -tc="DualizeRUNC*")
```

Python wrapper: `cmake --build build --target InstallRuncLib` builds `librunc.so` and copies it into `src/pyrunc/`, where `src/pyrunc/runc.py` loads it via `ctypes` (requires numpy). `runc.py` contains example `test_*` functions; `test_enumerate_from_file` has a hardcoded absolute path.

Formatting: `.clang-format` (LLVM-based, 4-space indent, 120 columns).

## What is actually built

Most of the tree is **not compiled**. Only sources listed (uncommented) in each `CMakeLists.txt` are built; recent commits deliberately pruned targets. Before assuming a file is live, check its CMakeLists:

- `src/common` → static lib `common` (bit containers, IO, graph utils). `CSVLoader`, `Registrator` are excluded (headers used by `TestUtils`).
- `src/dual` → static lib `dual` (RUNC, RuncFixed, PUNC, AOT1, pruning, cover collectors, weighters) and shared lib `runc` (`RuncExecutor.cpp`). `GeneticDualizer.h` and `FrequentInfrequentEnumerator.h` are listed in `SOURCE_FILES` but are dead leftovers of the poset code.
- `src/tests`: `TestDual` builds only `RUNCTests.cpp`; `TestUtils` builds all of its files (note `CSVTests.cpp` includes `CSVLoader.h`, whose `.cpp` is excluded from `common`, so it may fail to link).
- `legacy/` holds unmaintained, unbuilt code and data moved out of the tree (logical-analysis classifiers `src/logical`, poset dualization, `dualizer` executable, Boost-based `Configurable`/`logging_utils`, `3rd-party/` reference implementations, experiments, configs, mathematica notebooks, `.int` datasets); see `legacy/README.md`.

Re-enabling an excluded file usually means re-adding it (and its dependencies) to the relevant `SOURCE_FILES` list.

## RUNC architecture

- `DualizationBacktrackAlgorithmBase` (`src/dual`) holds the input `bit_matrix`, `loadMatrix(file)`, and two callback hooks, `InnerCallback` and `CoverCallback` (`IDualizationCallback::Call(DualizationNode&)`). Algorithms report each inner node / found cover through these; `Counters` and the collectors in `CoverCollector`/`MostCoverCollector` are callback implementations.
- `Runc::RuncGlobalState` (`Runc.h/.cpp`) implements the algorithm, selected by `AlgorithmType` (`OPT`, `RUNC`, `RUNCM`; default `RUNCM`). It keeps a stack of `RuncLocalState` (uncovered rows, support rows, remaining columns) and is **iterative, not recursive**: `startDualize()` sets up, `doStep()` advances one step, so enumeration can be paused and resumed.
- That resumability is what the Python API relies on: `RuncExecutor.cpp` exports `init_dualizer` / `set_input_row` / `enumerate_covers` / `release_dualizer` as `extern "C"`. `enumerate_covers` fills a caller buffer with `[size, col...]` records until there is no room for another `width+1` entries, then returns the count; repeated calls continue where the previous one stopped, and a return of 0 means enumeration is finished.
- Test inputs `*.hg` are hypergraph/matrix files: one row per line, space-separated column indices of ones. Filenames follow `rows_cols_expectedCoverCount` (e.g. `15_20_275.hg` has 275 minimal covers), which the tests assert.

## Workflow

Changes go through GitHub issues and pull requests (issue first, branch `type/N_description`, commits `[#N] type: Description` without trailers, draft PR with `Closes #N`, the owner merges). Details are in the `github` and `issue-template` skills under `.claude/skills/`; the PR body follows `.github/pull_request_template.md`.

## Conventions

- Many source comments are in Russian.
- `datasets/` holds `.hg` input matrices; logical-analysis configs, experiments and `.int` datasets are in `legacy/`.
