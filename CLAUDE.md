# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

C++17 research code for **dualization** algorithms: enumerating all minimal covers of a Boolean matrix (equivalently, minimal transversals of a hypergraph), plus generalizations over products of partially ordered sets. The actively maintained piece is the **RUNC** family of algorithms, exposed to Python through a C ABI shared library. Literature behind the algorithms is listed in `docs/references.md` (the owner's own paper and dissertation are in `docs/references/`). License: MIT (`LICENSE`).

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
- All tests pass under the `dev` and `asan` presets (since #21); keep it that way.

Python wrapper: `cmake --build build --target InstallRuncLib` builds `librunc.so` and copies it into `src/pyrunc/`, where `src/pyrunc/runc.py` loads it via `ctypes` (requires numpy). `runc.py` contains example `test_*` functions; `test_enumerate_from_file` has a hardcoded absolute path.

Formatting: `.clang-format` (LLVM-based, 4-space indent, 120 columns).

## What is actually built

Only sources listed in each `CMakeLists.txt` are built. Before assuming a file is live, check its CMakeLists:

- `src/common` → `dual::common` (bit containers, IO, graph utils). `CSVLoader.cpp` is not compiled (its header is used by `TestUtils`).
- `src/dual` → `dual::dual` (RUNC, RuncFixed, PUNC, AOT1, pruning, cover collectors, weighters) and `dual::runc` (`RuncExecutor.cpp`). `GeneticDualizer.h` and `FrequentInfrequentEnumerator.h` are dead leftovers of the poset code, not referenced by CMake.
- `src/tests`: `TestDual` builds `RUNCTests.cpp`, `RuncApiTests.cpp`, `RuncDifferentialTests.cpp` (with `RuncExecutor.cpp` compiled in) and `AOT1PUNCTests.cpp`; `TestRuncFixedO2` builds its own `-O2` copy of `RuncFixed.cpp` against `dual::common` only; `TestUtils` builds all of its files.
- `legacy/` holds unmaintained, unbuilt code and data moved out of the tree (logical-analysis classifiers `src/logical`, poset dualization, `dualizer` executable, Boost-based `Configurable`/`logging_utils`, `3rd-party/` reference implementations, experiments, configs, mathematica notebooks, `.int` datasets); see `legacy/README.md`.

Re-enabling an excluded file usually means adding it (and its dependencies) to the source/header lists of the relevant `CMakeLists.txt`.

## RUNC architecture

- `DualizationBacktrackAlgorithmBase` (`src/dual`) holds the input `bit_matrix`, `loadMatrix(file)`, and two callback hooks, `InnerCallback` and `CoverCallback` (`IDualizationCallback::Call(DualizationNode&)`). Algorithms report each inner node / found cover through these; `Counters` and the collectors in `CoverCollector`/`MostCoverCollector` are callback implementations.
- `Runc::RuncGlobalState` (`Runc.h/.cpp`) implements the algorithm, selected by `AlgorithmType` (`OPT`, `RUNC`, `RUNCM`; default `RUNCM`). It keeps a stack of `RuncLocalState` (uncovered rows, support rows, remaining columns) and is **iterative, not recursive**: `startDualize()` sets up, `doStep()` advances one step, so enumeration can be paused and resumed.
- That resumability is what the Python API relies on: `RuncExecutor.cpp` exports `init_dualizer` / `set_input_row` / `enumerate_covers` / `release_dualizer` as `extern "C"`. `enumerate_covers` fills a caller buffer with `[size, col...]` records until there is no room for another `width+1` entries, then returns the count; repeated calls continue where the previous one stopped, a return of 0 means enumeration is finished, and negative returns are error codes (e.g. buffer smaller than `width+1`). The full contract (input semantics, restart on input change) is the comment in `RuncExecutor.h`; the C++ side is documented at `RuncGlobalState` in `Runc.h`.
- Test inputs `*.hg` are hypergraph/matrix files: one row per line, space-separated column indices of ones. Filenames follow `rows_cols_expectedCoverCount` (e.g. `15_20_275.hg` has 275 minimal covers), which the tests assert.

## Workflow

Changes go through GitHub issues and pull requests (issue first, branch `type/N_description`, commits `[#N] type: Description` without trailers, draft PR with `Closes #N`, the owner merges). Details are in the `github`, `issue-template` and `build-and-test` skills under `.claude/skills/`; the PR body follows `.github/pull_request_template.md`.

## Conventions

- Comments are in English; sources are UTF-8.
- `datasets/` holds `.hg` input matrices; logical-analysis configs, experiments and `.int` datasets are in `legacy/`.
