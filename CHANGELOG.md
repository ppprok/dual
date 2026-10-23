# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

The first release is planned as **0.1.0** (the version in `project(dual VERSION ...)` in
`CMakeLists.txt`). It turns the research code into a reusable library (epic
[#7](https://github.com/ppprok/dual/issues/7)).

### Added

- Python bindings for FK: `algorithm="fk"` in every function of the `dual` package and in
  `Dualizer` (resumable, one `FK::GlobalState::next()` per cover), `Dualizer.next_cover()` and
  `Dualizer.all_covers()`, and the duality test `dual.is_dual(h, g)` returning `None` or a
  `DualityWitness(kind, columns)` ([#46](https://github.com/ppprok/dual/issues/46)).
- Khachiyan's incremental dualization `FK::GlobalState` (`<dual/fk.h>`): the Fredman–Khachiyan
  duality test (algorithm B) `FK::isDual` with a witness of non-duality, `next()` for one more
  cover and `dualize()` for all of them ([#43](https://github.com/ppprok/dual/issues/43)).
- `.clang-tidy` with a curated check set, a blocking clang-tidy CI job and the `DUAL_CLANG_TIDY`
  option; `.git-blame-ignore-revs` for the mass-formatting commit
  ([#42](https://github.com/ppprok/dual/pull/42)).
- Python package `dual` on pybind11 + scikit-build-core (`pip install .`): `enumerate_covers`,
  `minimal_covers`, `count_covers`, batched and resumable enumeration for RUNC/RUNCM/OPT, AOT1 and
  PUNC variants, numpy input, type stubs; the version comes from CMake
  ([#40](https://github.com/ppprok/dual/pull/40)).
- Contributor docs: `CONTRIBUTING.md`, this changelog, `CITATION.cff`, issue templates, and
  publication of the Doxygen HTML to GitHub Pages ([#15](https://github.com/ppprok/dual/issues/15)).
- CI on GitHub Actions: build and `ctest` with the `dev` and `release` presets on Linux (gcc,
  clang), macOS and Windows (MSVC), the `asan` preset, Doxygen with warnings as errors, a
  clang-format check of changed lines and a Python job
  ([#39](https://github.com/ppprok/dual/pull/39)).
- Doxygen comments for the whole public API of `dual::common`, `dual::dual` and the C ABI,
  `docs/Doxyfile`, `docs/mainpage.md` and the `DUAL_BUILD_DOCS` option with the `dual_docs` target;
  comments lost to encoding damage were restored from history
  ([#35](https://github.com/ppprok/dual/pull/35)).
- `dual_consumer` CTest test: installs the build and builds a `find_package(dual)` project against
  it; tests for `WeightBoundsPruningCallback` and `CoveredRowsWeighter`
  ([#37](https://github.com/ppprok/dual/pull/37)).
- Project `README.md` with a worked example, C++ and Python quickstarts, and short READMEs in every
  maintained directory ([#33](https://github.com/ppprok/dual/pull/33)).
- English rendering of the research task statement, `docs/research-tasks.md`
  ([#32](https://github.com/ppprok/dual/pull/32)).
- Resumable RUNC API: `resetDualize()`, `isDualizeFinished()`, a const `get_matrix()`, and
  defined results for empty input (no rows: one empty cover; a zero row: no covers); negative
  `RUNC_ERROR_*` return codes in the C ABI of `librunc.so`
  ([#27](https://github.com/ppprok/dual/pull/27)).
- Regression tests for the container, input, AOT1/PUNC, RuncFixed and RUNC API fixes, and
  differential tests on random matrices (RUNC variants against brute force, AOT1 and PUNC against
  RUNCM)
  ([#25](https://github.com/ppprok/dual/pull/25), [#26](https://github.com/ppprok/dual/pull/26),
  [#27](https://github.com/ppprok/dual/pull/27), [#28](https://github.com/ppprok/dual/pull/28),
  [#38](https://github.com/ppprok/dual/pull/38)).
- Modern target-based CMake: targets `dual::common`, `dual::dual`, `dual::runc`; options
  `DUAL_BUILD_TESTS`, `DUAL_ENABLE_SANITIZERS`, `DUAL_WARNINGS_AS_ERRORS`, `DUAL_INSTALL`,
  `DUAL_BUILD_PYTHON`; presets `dev`, `release`, `asan`; doctest via `FetchContent` with each case
  registered in CTest; `install()` and a `dualConfig.cmake` package for `find_package(dual)`
  ([#18](https://github.com/ppprok/dual/pull/18)).
- MIT `LICENSE` and `docs/references.md` with the literature behind the algorithms
  ([#17](https://github.com/ppprok/dual/pull/17)).
- Contributor workflow for Claude Code: `CLAUDE.md`, skills for GitHub issues and pull requests,
  the pull request template, and a build-and-test skill
  ([#6](https://github.com/ppprok/dual/pull/6), [#20](https://github.com/ppprok/dual/pull/20)).

### Changed

- Library layout: public headers moved to `include/dual/` with snake_case names (included as
  `<dual/name.h>`), internal headers stay in `src/`; all C++ code is in namespace `dual`, with
  algorithm state in `dual::Runc`, `dual::AOT1` and `dual::PUNC`; `DUAL_WARNINGS_AS_ERRORS` applies
  to GCC and Clang only; `mutable_matrix()` replaces non-const matrix access
  ([#42](https://github.com/ppprok/dual/pull/42)).
- The project language is English: comments, skills, the pull request template and documentation
  ([#16](https://github.com/ppprok/dual/pull/16), [#32](https://github.com/ppprok/dual/pull/32)).
- Sources converted from windows-1251 to UTF-8 ([#16](https://github.com/ppprok/dual/pull/16)).
- `AOT1::GlobalState` and `PUNC::GlobalState` override `dualize()` and can be used through
  `DualizationBacktrackAlgorithmBase`; `std::random_shuffle` replaced with `std::shuffle`, and
  `MostCoverCollectorOptions::Seed` added ([#28](https://github.com/ppprok/dual/pull/28),
  [#39](https://github.com/ppprok/dual/pull/39)).
- `bit_matrix::set` grows the matrix when a bit lands beyond the row stride of a non-last row,
  instead of corrupting the next row ([#26](https://github.com/ppprok/dual/pull/26)).
- The C ABI header `RuncExecutor.h` is a real C header; `set_input_row` validates its arguments,
  and rows may be given in any order ([#27](https://github.com/ppprok/dual/pull/27)).
- `RuncFixedDualize<T>()` is declared in the header and instantiated for `bits32`/`bits64` in the
  library; `WeightBoundsPruning.cpp` is compiled into `dual::dual`
  ([#37](https://github.com/ppprok/dual/pull/37)).
- `librunc` exports all symbols on Windows, so MSVC produces an import library
  ([#39](https://github.com/ppprok/dual/pull/39)).

### Fixed

- Memory defects in the common containers: `chunk` offsets overflowing `int` when `realloc` moved
  large buffers, `blob::resize` on an empty blob, `bit_mask::subset_cmp`, `bit_chunk::get_last`,
  `chunk::resize` and the byte size in `bit_vector::zero`
  ([#25](https://github.com/ppprok/dual/pull/25)).
- Silent corruption of the input matrix: the first `bit_matrix::set` on an empty matrix, empty
  first lines in `.hg` files, the file size estimate, and integer overflow in
  `IntegersFileReader` (now `std::out_of_range`) ([#26](https://github.com/ppprok/dual/pull/26)).
- A dangling `CoverCallback` after `dualize(max_count)`, stale local-state references in RUNC, and
  `setRestrictions` writing to a copy ([#27](https://github.com/ppprok/dual/pull/27)).
- Use-after-free and too-small state reservations in AOT1 and PUNC, and missing explicit
  instantiations of `RuncFixed` ([#28](https://github.com/ppprok/dual/pull/28)).
- Link errors for installed consumers of `CoveredRowsWeighter` and `RuncFixedDualize`
  ([#37](https://github.com/ppprok/dual/pull/37)).
- Latent defects in common helpers: `bit_matrix::remove_rows`, `is_transitive_closed`, `streq`,
  `exclude`, `bit_mask::intersects`, `raw_vector::erase`, and uninitialised members in `blob` and
  `raw_vector` constructors ([#38](https://github.com/ppprok/dual/pull/38)).
- Build on macOS (libc++) and Windows (MSVC) ([#39](https://github.com/ppprok/dual/pull/39)).
- Infinite recursion in `CSVLoader::GetColumn() const`, a signed index in `CharMap`, an
  uninitialised value in `CSVColumn`, and leaks when `realloc` fails in the containers
  ([#42](https://github.com/ppprok/dual/pull/42)).

### Removed

- Unmaintained code and data moved to `legacy/` (not built): logical-analysis classifiers,
  dualization over products of posets, the `dualizer` executable, Boost-based helpers,
  third-party reference implementations, experiments and `.int` datasets
  ([#17](https://github.com/ppprok/dual/pull/17)); dead headers `GeneticDualizer.h`,
  `FrequentInfrequentEnumerator.h`, `errors.h` and the poset test data
  ([#37](https://github.com/ppprok/dual/pull/37)).
- PDFs of third-party papers, replaced with DOI links in `docs/references.md`
  ([#17](https://github.com/ppprok/dual/pull/17)).
- Global `include_directories` and the `spdlog` dependency from the build
  ([#18](https://github.com/ppprok/dual/pull/18)).

[Unreleased]: https://github.com/ppprok/dual/commits/master
