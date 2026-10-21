# src/tests

Unit tests on [doctest](https://github.com/doctest/doctest) v2.5.3 (fetched by `FetchContent` on the
first configure). Built when `DUAL_BUILD_TESTS` is ON, which is the default for a top-level build
and in all presets. Each doctest case is registered as a separate CTest test named
`<executable>.<case>`.

| Path | Contents |
|---|---|
| `CMakeLists.txt` | Fetches doctest, defines `dual_add_test()` and the `dual_test_data` target that copies `data/` next to the test executables. |
| `DualizationLib.Test/` | `TestDual` (links `dual::dual`): `RUNCTests.cpp` (`OPT`, `RUNC` and `RUNCM` on `data/*.hg` find the number of covers given in the file name; portions and buffers find all covers), `RuncApiTests.cpp` (resumable API and C ABI contract; `RuncExecutor.cpp` is compiled in), `RuncDifferentialTests.cpp` (RUNC variants vs brute force on random matrices, including empty matrices and zero rows), `AOT1PUNCTests.cpp` (all `AOT1` variants and `PUNC` vs RUNCM). `TestRuncFixedO2`: `RuncFixedO2Tests.cpp` with its own `-O2` copy of `RuncFixed.cpp`, linked against `dual::common` only, to check the explicit instantiations. |
| `UtilsLib.Test/` | `TestUtils` (links `dual::common`): bit operations, `bit_chunk`, container memory regressions (`ContainerMemoryTests.cpp`), input matrix reading and `bit_matrix` growth (`InputMatrixTests.cpp`), graph algorithms, integer file IO, `Registrator`, CSV loading. |
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

## Expected state

All tests pass under the `dev`, `release` and `asan` presets (no ASan/UBSan reports); any failure is a regression.

## Adding a test

Add the `.cpp` to the `SOURCES` of `dual_add_test(...)` in the subdirectory's `CMakeLists.txt`; a
file that is not listed is not compiled. Put input files into `data/` and open them as
`"data/<name>"`.
