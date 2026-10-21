# src/tests

Unit tests on [doctest](https://github.com/doctest/doctest) v2.5.3 (fetched by `FetchContent` on the
first configure). Built when `DUAL_BUILD_TESTS` is ON, which is the default for a top-level build
and in all presets. Each doctest case is registered as a separate CTest test named
`<executable>.<case>`.

| Path | Contents |
|---|---|
| `CMakeLists.txt` | Fetches doctest, defines `dual_add_test()` and the `dual_test_data` target that copies `data/` next to the test executables. |
| `DualizationLib.Test/` | `TestDual` (links `dual::dual`). Built from `main.cpp` and `RUNCTests.cpp` only: `OPT`, `RUNC` and `RUNCM` on `data/*.hg` must find the number of covers given in the file name; enumeration by portions (`dualize(100)`) and into a buffer must find all 12226 covers of `15_35_12226.hg`. `RuncFixedTests.cpp` is **not built** (it reads `../ao.test/datasets/`, which does not exist). |
| `UtilsLib.Test/` | `TestUtils` (links `dual::common`): bit operations, `bit_chunk`, graph algorithms, integer file IO, `Registrator`, CSV loading. |
| `data/` | Test inputs: `rows_cols_count.hg` matrices, `6_6_6.hg`, `integers.txt`, `iris.txt`, `winequality-red.csv`, `write_bm_test.hg`. The `*.json`, `*.info` and poset `*.csv` files are inputs of the poset tests in `legacy/` and are not used by the built tests. |

## Run

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev                                         # all tests
ctest --preset dev -R DualizeRUNC                          # by regex
ctest --preset dev -R 'TestUtils\.read_bit_matrix'         # one test (presets print output on failure)
```

The executables open `data/...` by relative path, so to run them by hand with doctest options
(`-tc`, `-s`, `-ltc`) start them from their build directory:

```bash
cd build/dev/src/tests
./TestDual -tc="DualizeRUNC*"
./TestUtils -ltc               # list test cases
```

## Known failures on `master`

- `TestUtils.load_CSV_and_create_columns` fails in every configuration.
- Under the `asan` preset (`cmake --preset asan && cmake --build --preset asan && ctest --preset asan`)
  all `TestDual` cases and `TestUtils.read_bit_matrix`, `TestUtils.write_bit_matrix`,
  `TestUtils.TopologicalOrderNotAcyclicTest` report heap-buffer-overflow / use-after-free.

These are pre-existing defects; a new failure is a regression.

## Adding a test

Add the `.cpp` to the `SOURCES` of `dual_add_test(...)` in the subdirectory's `CMakeLists.txt`; a
file that is not listed is not compiled. Put input files into `data/` and open them as
`"data/<name>"`.
