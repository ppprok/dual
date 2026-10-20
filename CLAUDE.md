# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

C++17 research code for **dualization** algorithms: enumerating all minimal covers of a Boolean matrix (equivalently, minimal transversals of a hypergraph), plus generalizations over products of partially ordered sets. The actively maintained piece is the **RUNC** family of algorithms, exposed to Python through a C ABI shared library. Papers behind the algorithms are in `docs/references/`.

## Build and test

CMake >= 3.16 (presets need >= 3.21), C++17. No external dependencies; doctest v2.5.3 is fetched with `FetchContent` (network on first configure).

```bash
cmake --preset dev                 # Debug + tests, Ninja, build dir build/dev (also: release, asan)
cmake --build --preset dev
ctest --preset dev                 # every doctest case is a separate CTest test: TestDual.<case>, TestUtils.<case>
ctest --preset dev -R DualizeRUNC  # subset by regex

# Without presets
cmake -S . -B build && cmake --build build -j && ctest --test-dir build
```

- Test executables live in `<build>/src/tests` and run from there: they open `data/...` by relative path, and the `dual_test_data` target copies `src/tests/data/` next to them. Running by hand: `cd build/dev/src/tests && ./TestDual -tc="DualizeRUNC*"`.
- Options: `DUAL_BUILD_TESTS` (ON when top-level), `DUAL_ENABLE_SANITIZERS` (ASan+UBSan, used by the `asan` preset), `DUAL_WARNINGS_AS_ERRORS` (OFF; the code still has many `-Wall -Wextra` warnings), `DUAL_INSTALL` (ON when top-level), `DUAL_BUILD_PYTHON` (placeholder until #13).
- Targets: `dual::common` (static, file `libdual_common.a`), `dual::dual` (static, RUNC and friends), `dual::runc` (shared `librunc.so`, C ABI). Own targets get flags via `dual_target_options()` in `cmake/DualTargetOptions.cmake`; no global `include_directories`.
- Install / package: `cmake --install build --prefix <dir>` installs libs, flat headers under `include/dual/` and `lib/cmake/dual/dualConfig.cmake`; consumers use `find_package(dual)` + `target_link_libraries(app PRIVATE dual::dual)` and `#include "Runc.h"`.
- clangd: `compile_commands.json` is generated in every build dir; `.clangd` points at `build/dev`, so configure the `dev` preset once.
- Known: `TestUtils.load_CSV_and_create_columns` fails (pre-existing); under the `asan` preset RUNC and several IO tests report heap-buffer-overflow / use-after-free (pre-existing bugs, e.g. `bit_chunk::assign_or` from `Runc::RuncLocalState::CollectRestCols`).

Python wrapper: `cmake --build build --target InstallRuncLib` builds `librunc.so` and copies it into `src/pyrunc/`, where `src/pyrunc/runc.py` loads it via `ctypes` (requires numpy). `runc.py` contains example `test_*` functions; `test_enumerate_from_file` has a hardcoded absolute path.

Formatting: `.clang-format` (LLVM-based, 4-space indent, 120 columns).

## What is actually built

Most of the tree is **not compiled**. Only sources listed (uncommented) in each `CMakeLists.txt` are built; recent commits deliberately pruned targets. Before assuming a file is live, check its CMakeLists:

- `src/common` → static lib `common` (bit containers, IO, graph utils). `Configurable`, `CSVLoader`, `Registrator`, `logging_utils` are excluded.
- `src/dual` → static lib `dual` (RUNC, RuncFixed, PUNC, AOT1, pruning, cover collectors, weighters) and shared lib `runc` (`RuncExecutor.cpp`). Poset-based code (`PosetsDualizationTask`, `MIGRAS`, `GraphPoset`, `IntervalLattice`, `NumberChain`, `FrequentInfrequent*`, `GeneticDualizer`, …) and `main.cpp`/`dualizer` executable are excluded.
- `src/logical` (logical-analysis classifiers, `logcls`) is not part of the build (not referenced from `src/CMakeLists.txt`); it depends on Boost, which the build does not look for.
- `src/tests`: `TestDual` builds only `RUNCTests.cpp`; `TestUtils` builds all of its files (note `CSVTests.cpp` includes `CSVLoader.h`, whose `.cpp` is excluded from `common`, so it may fail to link); `LogicalAnalysisLib.Test` is disabled.
- `3rd-party/` (reference implementations of other dualization algorithms: BEGK, BMR, DL, KS, shd31, mtminer) and the boost submodules are not referenced from the root `CMakeLists.txt`.

Re-enabling an excluded file usually means adding it (and its dependencies) to the source/header lists of the relevant `CMakeLists.txt`.

## RUNC architecture

- `DualizationBacktrackAlgorithmBase` (`src/dual`) holds the input `bit_matrix`, `loadMatrix(file)`, and two callback hooks, `InnerCallback` and `CoverCallback` (`IDualizationCallback::Call(DualizationNode&)`). Algorithms report each inner node / found cover through these; `Counters` and the collectors in `CoverCollector`/`MostCoverCollector` are callback implementations.
- `Runc::RuncGlobalState` (`Runc.h/.cpp`) implements the algorithm, selected by `AlgorithmType` (`OPT`, `RUNC`, `RUNCM`; default `RUNCM`). It keeps a stack of `RuncLocalState` (uncovered rows, support rows, remaining columns) and is **iterative, not recursive**: `startDualize()` sets up, `doStep()` advances one step, so enumeration can be paused and resumed.
- That resumability is what the Python API relies on: `RuncExecutor.cpp` exports `init_dualizer` / `set_input_row` / `enumerate_covers` / `release_dualizer` as `extern "C"`. `enumerate_covers` fills a caller buffer with `[size, col...]` records until there is no room for another `width+1` entries, then returns the count; repeated calls continue where the previous one stopped, and a return of 0 means enumeration is finished.
- Test inputs `*.hg` are hypergraph/matrix files: one row per line, space-separated column indices of ones. Filenames follow `rows_cols_expectedCoverCount` (e.g. `15_20_275.hg` has 275 minimal covers), which the tests assert.

## Workflow

Changes go through GitHub issues and pull requests (issue first, branch `type/N_description`, commits `[#N] type: Description` without trailers, draft PR with `Closes #N`, the owner merges). Details are in the `github` and `issue-template` skills under `.claude/skills/`; the PR body follows `.github/pull_request_template.md`.

## Conventions

- Many source comments are in Russian.
- `configs/` and `experiments/` hold `.info` configs and datasets for the (currently unbuilt) logical-analysis experiments; `datasets/` and `test-datasets/` are input data.
