---
name: build-and-test
description: Building and testing this repo (CMake presets dev/release/asan, ctest, doctest, sanitizers, find_package check). Triggers — "build it", "run the tests", "run a test", "check under sanitizers", "ASan", "why doesn't it build", "check before the PR", "compile_commands", "clangd can't see it".
---

# Build and test

The tools — `cmake`, `ninja`, `clang-format`, `clang-tidy`, `clangd` — are installed via `uv tool` into `~/.local/bin`.
If one is missing, use `uv tool install <name>`, not `apt`. The compiler is g++ 11 (there is no clang compiler).
There are no external dependencies (Boost and spdlog are not needed); doctest is downloaded by `FetchContent` on the
first configure, which needs network access.

## Main loop

```bash
cmake --preset dev && cmake --build --preset dev && ctest --preset dev   # build/dev, Debug
ctest --preset dev -R DualizeRUNC                                        # subset by regex
ctest --preset dev --output-on-failure -R 'TestUtils\.read_bit_matrix'   # one test with output
```

Presets: `dev` (Debug), `release`, `asan` (RelWithDebInfo + ASan/UBSan, `DUAL_ENABLE_SANITIZERS=ON`).
Each doctest case is a separate CTest test `TestDual.<case>` / `TestUtils.<case>`. One more CTest test,
`dual_consumer`, installs the build into `<build>/src/tests/consumer/prefix` and builds/runs the
`find_package(dual)` project in `src/tests/consumer/` against it (no network needed); it catches symbols that
link inside the build tree but not for an installed consumer.

By hand, with doctest options (`-tc`, `-s`, `-d`): **only from the tests directory** — the binaries open
`data/...` by relative path, and the `dual_test_data` target copies the data there:

```bash
cd build/dev/src/tests && ./TestDual -tc="DualizeRUNCTest4" -s
```

In a git worktree the preset puts the build into `<worktree>/build/<preset>` — each worktree has its own build,
and `build/` is in `.gitignore`.

## Known state (update when fixed)

- `ctest --preset dev` and `ctest --preset asan` pass fully, with no ASan/UBSan reports (#21 fixed the `chunk`
  rebase, `blob::resize`, `bit_mask::subset_cmp`, `bit_chunk::get_last` and `bit_vector::zero` — they caused the
  `load_CSV_and_create_columns`, RUNC and IO `bit_matrix` failures under `asan`).
- Live code is free of g++ `-Wall -Wextra` warnings in all three presets, and the presets build with
  `DUAL_WARNINGS_AS_ERRORS=ON` (`-Werror`), so a new warning breaks the build (#12). Plain
  `cmake -S . -B build` keeps it OFF.
- `.clang-tidy` reports nothing on live code (#12).

Compare against this list: a new failure is yours; an old one is not to be fixed along the way, but mentioned in the PR.

## Before a PR

1. `ctest --preset dev` — no more failures than in the list above.
2. If you touched the algorithms or the bit containers — also `cmake --preset asan && cmake --build --preset asan && ctest --preset asan`
   and compare the number of failures with `master`.
3. `clang-format -i` on the changed `.cpp/.h` under `include/` and `src/` (config `.clang-format`; do not format
   `legacy/`). A pure mass-reformatting commit goes into `.git-blame-ignore-revs`.
4. clang-tidy on the changed code: `run-clang-tidy.py -p build/dev -quiet 'src/(common|dual|tests)/'` (uses
   `build/dev/compile_commands.json`; headers in `include/dual` are checked through the sources), or configure a
   separate build dir with `-DDUAL_CLANG_TIDY=ON`. It must stay clean; suppress a single finding only with
   `// NOLINT(check): reason`.
5. If you touched public headers or `docs/` — `mkdir -p build && doxygen docs/Doxyfile` must print no warnings.
6. If you changed the CMake export or public headers — `ctest --preset dev -R dual_consumer` checks an installed
   consumer (it is part of the full `ctest` run too). A new public symbol that consumers need should be used in
   `src/tests/consumer/main.cpp`.
7. If you added a `.cpp/.h` — add it to the source list of its `CMakeLists.txt`, otherwise the file is not built
   and clangd does not see it. A header that library users need (or that a public header includes) is public:
   `include/dual/<snake_case>.h`, listed in `DUAL_*_PUBLIC_HEADERS`, included as `<dual/...>`, code in namespace
   `dual`. Other headers stay internal next to their sources in `src/`.

## Algorithm correctness

The expected number of minimal covers is encoded in the file name `src/tests/data/<rows>_<cols>_<count>.hg`.
The `OPT`, `RUNC`, `RUNCM` variants must produce the same set of covers — when changing an algorithm,
compare them with each other on random matrices and against brute force for small `n`.

## clangd

`.clangd` points at `build/dev/compile_commands.json` — running `cmake --preset dev` once is enough.
