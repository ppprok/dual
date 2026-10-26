# src/tests

Unit tests on [doctest](https://github.com/doctest/doctest) v2.5.3 (fetched by `FetchContent` on the
first configure). Built when `DUAL_BUILD_TESTS` is ON, which is the default for a top-level build
and in all presets. Each doctest case is registered as a separate CTest test named
`<executable>.<case>`.

| Path | Contents |
|---|---|
| `CMakeLists.txt` | Fetches doctest, defines `dual_add_test()` and the `dual_test_data` target that copies `data/` next to the test executables. |
| `DualizationLib.Test/` | `TestDual` (links `dual::dual`): `RUNCTests.cpp` (`OPT`, `RUNC` and `RUNCM` on `data/*.hg` find the number of covers given in the file name; portions and buffers find all covers), `RuncApiTests.cpp` (resumable API and C ABI contract; `runc_executor.cpp` is compiled in), `RuncDifferentialTests.cpp` (RUNC variants vs brute force on random matrices, including empty matrices and zero rows), `AOT1PUNCTests.cpp` (all `AOT1` variants and `PUNC` vs RUNCM), `PruningTests.cpp` (`WeightBoundsPruningCallback` and `CoveredRowsWeighter` on RUNC-M: the covers kept under fixed or dynamic weight bounds equal the full enumeration filtered by weight), `FKTests.cpp` (`FK::GlobalState` against the file names and RUNCM on `data/*.hg`, against RUNCM and brute force on random matrices, `next()` (also that it continues the run of `dualize()` cover by cover), matrices wider than 64 columns, and corner cases; `FK::isDual` on dual and non-dual pairs), `AlgorithmMatrixTests.cpp` (every algorithm with every option — `Runc` OPT/RUNC/RUNCM incl. `dualize(max_count)`, the six `AOT1` types, `PUNC` with and without orders, `FK` `dualize()`/`next()`, `RuncFixed<bits32/bits64>` incl. callbacks and `DualizeIntervals` — on every data file against the count in its name and RUNCM, and on small random matrices against brute force; stop after N covers, callback exceptions and a consistent search tree for all `DualizationBacktrackAlgorithmBase` algorithms), `CallbackTests.cpp` (`ColumnsWeighterCallback`/`CoveredRowWeighterCallback` on several algorithms against brute force filtered by weight, `MostCoverCollector` with a weighter, `BitCoverCollectorCallback`, `CoverCollector`, `ChainCallback`, `DualizationCounters`, the cover file writers, restrictions, `CoveredRowsWeighter` targets, `BoolMatrixPreparation`). `DualTestUtils.h` holds the shared helpers (matrices as rows, brute force, random matrices, data files with their expected counts, `collect()`); `RuncTestUtils.h` adds the RUNC/C ABI ones. `TestRuncFixedO2`: `RuncFixedO2Tests.cpp` with its own `-O2` copy of `runc_fixed.cpp`, linked against `dual::common` only, to check the explicit instantiations. |
| `UtilsLib.Test/` | `TestUtils` (links `dual::common`): bit operations, `bit_chunk`, container memory regressions (`ContainerMemoryTests.cpp`), input matrix reading and `bit_matrix` growth (`InputMatrixTests.cpp`), graph algorithms, integer file IO and `io_utils` (with their error paths), `Registrator`, CSV loading. |
| `consumer/` | `dual_consumer` CTest test (registered when `DUAL_INSTALL` is ON): `RunConsumerTest.cmake` installs the current build into `<build>/src/tests/consumer/prefix`, then configures, builds and runs the stand-alone `find_package(dual)` project in this directory (`main.cpp` includes `<dual/...>` headers and uses `dual::RuncFixedDualize` and `dual::CoveredRowsWeighter`), so symbols that only fail to link outside the build tree are caught. Needs no network; under `DUAL_ENABLE_SANITIZERS` the consumer is linked with `-fsanitize=address,undefined`. |
| `data/` | Test inputs: `rows_cols_count.hg` matrices, `6_6_6.hg`, `integers.txt`, `iris.txt`, `winequality-red.csv`, `write_bm_test.hg`. The poset inputs (`*.json`, `*.info`, poset `*.csv`) moved to `legacy/src/tests/data/`. |

## Run

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev                                         # all tests
ctest --preset dev -R DualizeRUNC                          # by regex
ctest --preset dev -R 'TestUtils\.read_bit_matrix'         # one test (presets print output on failure)
ctest --preset dev -R dual_consumer                        # install + find_package(dual) consumer build
```

The executables open `data/...` by relative path, so to run them by hand with doctest options
(`-tc`, `-s`, `-ltc`) start them from their build directory:

```bash
cd build/dev/src/tests
./TestDual -tc="DualizeRUNC*"
./TestUtils -ltc               # list test cases
```

## Coverage

```bash
cmake --preset coverage && cmake --build --preset coverage
find build/coverage -name '*.gcda' -delete && ctest --preset coverage
gcovr build/coverage          # settings in gcovr.cfg; gcovr: uv tool install gcovr
```

## Expected state

All tests pass under the `dev`, `release` and `asan` presets (no ASan/UBSan reports); any failure is a regression.

## Adding a test

Add the `.cpp` to the `SOURCES` of `dual_add_test(...)` in the subdirectory's `CMakeLists.txt`; a
file that is not listed is not compiled. Put input files into `data/` and open them as
`"data/<name>"`. Include public headers as `<dual/...>` and internal ones relative to `src/`
(`"common/string_utils.h"`); test files say `using namespace dual;` after the includes. Tests are
built with `-Wall -Wextra` and, in the presets, `-Werror`.
