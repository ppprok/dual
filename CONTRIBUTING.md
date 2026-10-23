# Contributing to dual

Thanks for your interest in `dual`. This page describes how to build and test the project, the code
and documentation conventions, and how a change travels from an issue to a merged pull request.
Issues, pull requests, commit messages, code comments and documentation are written in **English**.

## Build and test

Requirements: a C++17 compiler (GCC, Clang, Apple Clang or MSVC), CMake 3.21+ for the presets
(3.16+ without them) and Ninja. doctest is fetched with `FetchContent` on the first configure, so
that step needs network access; there are no other dependencies.

```bash
cmake --preset dev                  # Debug + tests, build dir build/dev
cmake --build --preset dev
ctest --preset dev                  # every doctest case is a CTest test: TestDual.<case>, TestUtils.<case>
ctest --preset dev -R DualizeRUNC   # a subset by regex
```

Other presets: `release`, and `asan` (RelWithDebInfo with AddressSanitizer and UBSan). Run the
`asan` preset whenever you touch the algorithms or the bit containers:

```bash
cmake --preset asan && cmake --build --preset asan && ctest --preset asan
```

The test executables open `data/...` by relative path, so to run one by hand with doctest options
start it from its build directory: `cd build/dev/src/tests && ./TestDual -tc="DualizeRUNC*" -s`.
The `dual_consumer` CTest test installs the build into a scratch prefix and builds a
`find_package(dual)` project against it; it catches symbols that link only inside the build tree.

All tests pass under `dev`, `release` and `asan`; any failure is a regression. More details are in
[`src/tests/README.md`](src/tests/README.md).

### Python package

The Python package `dual` (pybind11 + scikit-build-core, sources in `python/`) is built and tested
with pip and pytest:

```bash
python -m pip install ".[test]"
python -m pytest python/tests
```

### API documentation

```bash
cmake --preset dev -DDUAL_BUILD_DOCS=ON && cmake --build --preset dev --target dual_docs   # build/dev/docs/html
# or directly, from the repository root:
mkdir -p build/docs && doxygen docs/Doxyfile   # HTML in build/docs/html
```

## Code style

- Format C++ with [`.clang-format`](.clang-format) (LLVM-based, 4-space indent, 120 columns):
  `clang-format -i <files>`, or `git clang-format origin/master` to format only the lines you
  changed. CI checks the changed lines with clang-format 23.1.3. Do not format `legacy/`.
- Static analysis: [`.clang-tidy`](.clang-tidy) runs clean on the live code and CI fails on any
  finding. Run it locally after configuring the `dev` preset:
  `run-clang-tidy.py -p build/dev -quiet 'src/(common|dual|tests)/'`. Suppress a single finding
  only with `// NOLINT(check): reason`.
- All C++ code is in namespace `dual` (algorithm state in `dual::Runc`, `dual::AOT1`,
  `dual::PUNC`); only the `extern "C"` API of `librunc` is outside it.
- [`.editorconfig`](.editorconfig) sets UTF-8, LF line endings and indentation for the other file
  types.
- Comments are in English; sources are UTF-8.
- Public headers live in `include/dual/` with snake_case names and are included as
  `<dual/name.h>`; internal headers stay next to their sources in `src/`. A header that a public
  header includes must itself be public.
- Only files listed in a `CMakeLists.txt` are built. When you add a `.cpp` or `.h`, add it to the
  source or header list of its target (`DUAL_COMMON_PUBLIC_HEADERS` / `DUAL_DUAL_PUBLIC_HEADERS` for
  public headers), otherwise it is neither compiled nor installed.
- No debug output, and no `TODO`/`FIXME` without an issue number.
- `legacy/` is unmaintained and unbuilt; do not fix things there along the way.

## Tests and datasets

- Tests use [doctest](https://github.com/doctest/doctest). Add a test file to the `SOURCES` of
  `dual_add_test(...)` in the `CMakeLists.txt` of its test directory; each `TEST_CASE` becomes a
  separate CTest test automatically.
- Bug fixes come with a regression test that fails on the unfixed code.
- Test inputs go into `src/tests/data/` and are opened as `"data/<name>"`. Sample matrices for
  users go into `datasets/`.
- Matrices are `.hg` files: one row per line, the 0-based indices of the columns holding a one,
  separated by spaces. A file with a known answer is named **`rows_cols_count.hg`**, where `count` is
  the number of minimal covers (e.g. `15_20_275.hg`: 15 rows, 20 columns, 275 minimal covers); the
  tests read the expected count from the name, so the name must be right. Verify `count`
  independently (brute force for small matrices, or agreement of several algorithms).
- When you change an algorithm, check that `OPT`, `RUNC` and `RUNCM` (and the other algorithms)
  still produce the same set of covers; `RuncDifferentialTests.cpp` and `AOT1PUNCTests.cpp` do this
  on random matrices.

## Documentation

- Every public class, function, method, enum and typedef in the installed headers has a Doxygen
  comment (`///` with `@brief`, `@param`, `@return`, `@throws` where they apply). The docs job in CI
  runs Doxygen with warnings as errors, so an undocumented public symbol fails the build.
- [`docs/Doxyfile`](docs/Doxyfile) reads every header in `include/dual/`, so a new public header
  is documented (and checked) automatically.
- Reproduce the CI docs check locally:

  ```bash
  mkdir -p build && printf '@INCLUDE = docs/Doxyfile\nWARN_AS_ERROR = FAIL_ON_WARNINGS\n' > build/Doxyfile.ci
  doxygen build/Doxyfile.ci
  ```

- The HTML is published to <https://ppprok.github.io/dual/> on every push to `master`
  ([`.github/workflows/docs.yml`](.github/workflows/docs.yml)).
- Update `README.md`, the per-directory READMEs and `CLAUDE.md` when behaviour, build options or
  the layout change, and add a line to [`CHANGELOG.md`](CHANGELOG.md) under `[Unreleased]`.

## Workflow: issue, branch, pull request

1. **Issue first.** Every change starts with a GitHub issue. Use the issue templates; an issue has
   the sections *Context* (why, and what happens if it is not done), *DoD* (verifiable result, and
   what is out of scope), *Plan* (the steps and the forks in the road) and *Constraints and
   dependencies* (blocking issues, decisions needed from the owner). One pull request is one logical
   unit of work.
2. **Branch** from a fresh `master`: `type/N_description`, where `type` is one of `feature`, `bug`,
   `task`, `refactor`, `docs`, `N` is the issue number and the description is 2-3 English words,
   e.g. `bug/22_input-corruption`.
3. **Commits:** `[#N] type: Description` with a conventional type (`feat`, `fix`, `docs`,
   `refactor`, `test`, `ci`, `chore`, ...), an imperative one-sentence description, and a body of at
   most three sentences. No trailers.
4. **Pull request:** title `[#N] - type - Title`, body from
   [`.github/pull_request_template.md`](.github/pull_request_template.md) with `Closes #N` (this
   links the issue and closes it on merge). Open it as a **draft** while it is in progress, and fill
   in the checklist; leave items you could not verify unchecked and say why.
5. **Labels** on the issue track its state, exactly one at a time: `In progress` (work started) →
   `In review` (pull request waiting for review) → `Done` (merged) or `Rejected` (closed without
   being done). `Needs rework` marks a change sent back from review.
6. **Review and merge.** Reply to review comments in their threads. The owner reviews and merges;
   pull requests are merged with a merge commit after updating the branch from `master`.

## Continuous integration

[`.github/workflows/ci.yml`](.github/workflows/ci.yml) runs on every pull request and push to
`master`, and every job must be green before review:

| Job | What it checks |
|---|---|
| `linux-gcc`, `linux-clang`, `macos`, `windows-msvc` × `dev`, `release` | configure, build and `ctest` with the preset (including `dual_consumer`) |
| `linux-gcc asan` | the `asan` preset: tests under AddressSanitizer and UBSan |
| `docs` | Doxygen with warnings as errors |
| `clang-format` | the lines changed by the pull request are clang-formatted |
| `clang-tidy` | `.clang-tidy` on all live code; any finding fails |
| `python 3.10`, `python 3.12` | `pip install ".[test]"` and `pytest python/tests` |

Each job can be reproduced locally with the commands above.

## License

By contributing you agree that your contributions are licensed under the [MIT License](LICENSE).
