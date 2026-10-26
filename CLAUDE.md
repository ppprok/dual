# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

C++17 research code for **dualization** algorithms: enumerating all minimal covers of a Boolean matrix (equivalently, minimal transversals of a hypergraph), plus generalizations over products of partially ordered sets. The actively maintained piece is the **RUNC** family of algorithms, exposed to Python as the pybind11 package `dual` (`python/`) and through a C ABI shared library. Literature behind the algorithms is listed in `docs/references.md` (the owner's own paper and dissertation are in `docs/references/`). License: MIT (`LICENSE`).

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
- Options: `DUAL_BUILD_TESTS` (ON when top-level), `DUAL_ENABLE_SANITIZERS` (ASan+UBSan, used by the `asan` preset), `DUAL_WARNINGS_AS_ERRORS` (OFF by default so consumers/packagers with other compilers are not broken; ON in all presets — live code is free of `-Wall -Wextra` warnings with GCC, Clang and Apple Clang (checked by CI), keep it so; it does not add `/WX` on MSVC, where `/W4` still reports int-size conversions), `DUAL_CLANG_TIDY` (OFF; sets `CMAKE_CXX_CLANG_TIDY` to run `.clang-tidy` during the build, as errors with `DUAL_WARNINGS_AS_ERRORS`), `DUAL_INSTALL` (ON when top-level), `DUAL_BUILD_PYTHON` (OFF; adds `python/`, the pybind11 module `_dual`; `pip install .` turns it on), `DUAL_BUILD_DOCS` (OFF; adds target `dual_docs`, which runs Doxygen with `docs/Doxyfile` into `<build>/docs/html`).
- Targets: `dual::common` (static, file `libdual_common.a`), `dual::dual` (static, RUNC and friends), `dual::runc` (shared `librunc.so`, C ABI). Own targets get flags via `dual_target_options()` in `cmake/DualTargetOptions.cmake`; no global `include_directories`: `dual::common` and `dual::runc` export `include/` via `$<BUILD_INTERFACE:…/include>` / `$<INSTALL_INTERFACE:include>`.
- Install / package: `cmake --install build --prefix <dir>` installs libs, the public header tree `include/dual/` and `lib/cmake/dual/dualConfig.cmake`; consumers use `find_package(dual)` + `target_link_libraries(app PRIVATE dual::dual)` and `#include <dual/runc.h>`.
- clangd: `compile_commands.json` is generated in every build dir; `.clangd` points at `build/dev`, so configure the `dev` preset once.
- All tests pass under the `dev`, `release` and `asan` presets; keep it that way. `doxygen docs/Doxyfile` (run from the root after `mkdir -p build`) must stay at 0 warnings.

CI (`.github/workflows/ci.yml`, on PRs and pushes to `master`): `dev` + `release` presets (configure, build, `ctest`, so also `dual_consumer`) on ubuntu gcc, ubuntu clang, macOS and Windows MSVC; `asan` preset on ubuntu gcc; Doxygen with `WARN_AS_ERROR = FAIL_ON_WARNINGS`; `clang-format` 23.1.3 on the lines changed by the PR only (`legacy/` excluded); `clang-tidy` 23.1.3 with `.clang-tidy` on all live code via `build/dev/compile_commands.json` (blocking: any finding fails); a Python job (`pip install ".[test]"`, `pytest python/tests`, Python 3.10/3.12). Reproduce locally with the commands in the CI section of the `build-and-test` skill.
`.github/workflows/docs.yml` (push to `master`, manual) builds the Doxygen HTML and deploys it to GitHub Pages (https://ppprok.github.io/dual/; Pages source must be "GitHub Actions").

Python package `dual` (see the `python-bindings` skill and `python/README.md`): `pyproject.toml` (scikit-build-core + pybind11) builds `python/src/_dual.cpp` into `dual._dual`; the API is `python/dual/__init__.py` (`enumerate_covers`, `iter_batches`, `minimal_covers`, `count_covers`, `Dualizer`). Check with `pip install ".[test]" && pytest` (tests in `python/tests`, they read `src/tests/data`). All C++ includes of the bindings are in `_dual.cpp`. The old `ctypes` wrapper `src/pyrunc/runc.py` (over `librunc.so`, copied there by the `InstallRuncLib` target) is deprecated but kept as a C ABI example.

Formatting: `.clang-format` (LLVM-based, 4-space indent, 120 columns, include groups: main header, `<doctest.h>`/`<pybind11/...>`, `<std>`, `<dual/...>`, internal `"..."`) over `include/` and `src/` (not `legacy/`); the mass-formatting commit is in `.git-blame-ignore-revs`. Static analysis: `.clang-tidy` (curated bugprone/performance/clang-analyzer/modernize/readability set, exclusions justified in the file) runs clean: `run-clang-tidy.py -p build/dev -quiet 'src/(common|dual|tests)/'`; suppress a single finding only with `// NOLINT(check): reason`.

## What is actually built

Only sources listed in each `CMakeLists.txt` are built. Before assuming a file is live, check its CMakeLists:

- `include/dual/*.h` — the **public** headers (snake_case names, installed, documented by Doxygen, included everywhere as `<dual/name.h>`): bit containers (`bits`, `chunk`, `raw_vector`, `blob`, `bit_chunk`, `bit_vector`, `bit_mask`, `bit_matrix`, `containers`, `range`, `marked`), IO (`io_utils`, `integers_file_reader`, `integers_file_writer`), the framework and callbacks (`dualization_backtrack_algorithm_base`, `counters`, `dualization_counters`, `cover_collector`, `weighted_cover`, `most_cover_collector`, `file_writers`, `weighters`, `pruning`, `weight_bounds_pruning`), the algorithms (`runc`, `runc_fixed`, `aot1`, `punc`, `fk`) and the C ABI (`runc_executor.h`, a C header). A header that a public header includes must itself be public; each one compiles on its own.
- `src/common` → `dual::common` (implementation of the bit containers and IO, plus graph utils). **Internal** headers stay here and are not installed: `graph_algorithms.h`, `string_utils.h`, `math_utils.h`, `iterator_range.h`, `randomizer.h`, `registrator.h`, `csv_loader.h` (`csv_loader.cpp` is not compiled; the header is used by `TestUtils`). Tests include them as `"common/name.h"`.
- `src/dual` → `dual::dual` (RUNC, RuncFixed, PUNC, AOT1, FK (Khachiyan's incremental dualization, `fk.cpp`), pruning, cover collectors, weighters; internal header `bool_matrix_preparation.h`) and `dual::runc` (`runc_executor.cpp`).
- `python` → the extension module `_dual`, only when `DUAL_BUILD_PYTHON=ON` (pip sets it and builds only that target).
- `src/tests`: `TestDual` builds `RUNCTests.cpp`, `RuncApiTests.cpp`, `RuncDifferentialTests.cpp` (with `runc_executor.cpp` compiled in), `AOT1PUNCTests.cpp`, `PruningTests.cpp`, `CoverCollectorTests.cpp` and `FKTests.cpp`; `TestRuncFixedO2` builds its own `-O2` copy of `runc_fixed.cpp` against `dual::common` only; `TestUtils` builds all of its files. `dual_consumer` (when `DUAL_INSTALL` is ON) installs the build into `<build>/src/tests/consumer/prefix` and builds/runs `src/tests/consumer/` against it with `find_package(dual)`.
- `legacy/` holds unmaintained, unbuilt code and data moved out of the tree (logical-analysis classifiers `src/logical`, poset dualization, `dualizer` executable, Boost-based `Configurable`/`logging_utils`, `3rd-party/` reference implementations, experiments, configs, mathematica notebooks, `.int` datasets); see `legacy/README.md`.

Re-enabling an excluded file usually means adding it (and its dependencies) to the source/header lists of the relevant `CMakeLists.txt` (public headers: `DUAL_COMMON_PUBLIC_HEADERS` / `DUAL_DUAL_PUBLIC_HEADERS`).

## Namespaces

All C++ code (library and its public headers) is in namespace `dual`. The algorithms keep their own state in nested namespaces named after the algorithm, kept with their historical capitalization: `dual::Runc`, `dual::AOT1`, `dual::PUNC`, `dual::FK`; `dual::details` holds bit_chunk internals. The only code outside `dual` is the `extern "C"` API of `librunc` (`runc_executor.h/.cpp`; symbol names are part of the ABI used by `src/pyrunc/runc.py`). Tests write `using namespace dual;` after their includes; the consumer test qualifies names with `dual::`.

## RUNC architecture

- `DualizationBacktrackAlgorithmBase` (`<dual/dualization_backtrack_algorithm_base.h>`) holds the input `bit_matrix` (`setMatrix`, `loadMatrix(file)`, read-only `get_matrix()`, and `mutable_matrix()`, which counts as a matrix change and restarts a RUNC enumeration), and two callback hooks, `InnerCallback` and `CoverCallback` (`IDualizationCallback::Call(DualizationNode&)`). Algorithms report each inner node / found cover through these; `Counters` and the collectors in `CoverCollector`/`MostCoverCollector` are callback implementations.
- `Runc::RuncGlobalState` (`<dual/runc.h>`, `src/dual/runc.cpp`) implements the algorithm, selected by `AlgorithmType` (`OPT`, `RUNC`, `RUNCM`; default `RUNCM`). It keeps a stack of `RuncLocalState` (uncovered rows, support rows, remaining columns) and is **iterative, not recursive**: `startDualize()` sets up, `doStep()` advances one step, so enumeration can be paused and resumed.
- That resumability is what the Python API relies on: `runc_executor.cpp` exports `init_dualizer` / `set_input_row` / `enumerate_covers` / `release_dualizer` as `extern "C"`. `enumerate_covers` fills a caller buffer with `[size, col...]` records until there is no room for another `width+1` entries, then returns the count; repeated calls continue where the previous one stopped, a return of 0 means enumeration is finished, and negative returns are error codes (e.g. buffer smaller than `width+1`). The full contract (input semantics, restart on input change) is the comment in `<dual/runc_executor.h>`; the C++ side is documented at `RuncGlobalState` in `<dual/runc.h>`.
- Test inputs `*.hg` are hypergraph/matrix files: one row per line, space-separated column indices of ones. Filenames follow `rows_cols_expectedCoverCount` (e.g. `15_20_275.hg` has 275 minimal covers), which the tests assert.

## Workflow

Changes go through GitHub issues and pull requests (issue first, branch `type/N_description`, commits `[#N] type: Description` without trailers, draft PR with `Closes #N`, the owner merges). Details are in the `github`, `issue-template` and `build-and-test` skills under `.claude/skills/`; the PR body follows `.github/pull_request_template.md`.
Human-facing versions: `CONTRIBUTING.md` (build, style, workflow, CI) and `.github/ISSUE_TEMPLATE/`; record user-visible changes in `CHANGELOG.md` under `[Unreleased]`; citation metadata is in `CITATION.cff`.

## Conventions

- Comments are in English; sources are UTF-8.
- Language: everything is written in English — code comments, docs, issues, pull requests, commit messages and review comments. Russian originals that are kept for reference live in `legacy/`.
- `datasets/` holds `.hg` input matrices; logical-analysis configs, experiments and `.int` datasets are in `legacy/`.
