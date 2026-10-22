# dual: dualization algorithms

C++17 library that enumerates all **minimal covers of a Boolean matrix**. A set of columns *covers*
the matrix when every row has a one in at least one of those columns, and a cover is *minimal* when
no column can be dropped from it. Read the rows as hyperedges over the columns as vertices, and the
same task becomes enumerating the **minimal transversals (hitting sets) of a hypergraph**, also known
as dualization of a monotone Boolean function given in CNF. The number of minimal covers can grow
exponentially with the matrix size, so the algorithms here walk a backtracking search tree and
report covers one at a time instead of building the whole set at once. The generalization to
dualization over products of partially ordered sets lived in this repository too; it is
unmaintained and now sits in [`legacy/`](legacy/README.md).

A small example. Take the 3x3 matrix

```
       c0 c1 c2
r0  [  1  1  0 ]
r1  [  0  1  1 ]
r2  [  1  0  1 ]
```

No single column covers it (`{c0}` misses `r1`, `{c1}` misses `r2`, `{c2}` misses `r0`), every pair
does, and dropping a column from a pair breaks it. So the minimal covers are `{c0, c1}`,
`{c0, c2}` and `{c1, c2}`; `{c0, c1, c2}` is a cover but not a minimal one.

## Algorithms

All of them are in [`src/dual`](src/dual/README.md).

| Algorithm | Where | What it is |
|---|---|---|
| **RUNC**, **RUNCM**, **OPT** | `Runc::RuncGlobalState`, chosen by `Runc::AlgorithmType` (default `RUNCM`) | The RUNC family of asymptotically optimal algorithms (see [references](docs/references.md)). They differ only in how the next columns to branch on are chosen: `RUNC` takes the first uncovered row, `RUNCM` an uncovered row with the fewest remaining ones, `OPT` all columns left after removing dependent rows. All three must produce the same set of covers; the tests check this. This is the maintained part of the library. |
| **RuncFixed** | `RuncFixed<bits32>`, `RuncFixed<bits64>`, `RuncFixedDualize()` | Recursive RUNC-M for matrices of at most 32 or 64 columns, each row stored in one machine word. Also has `DualizeIntervals()`. Only `bits32` and `bits64` are instantiated. |
| **AOT1** | `AOT1::GlobalState`, variants `AO1`, `AO2`, `AO1K`, `AO2K`, `AO1M`, `AO2M` | Older asymptotically optimal algorithms of the AO family. Built, but not tested; see the status note below. |
| **PUNC** | `PUNC::GlobalState` | Algorithm with configurable row and column order (`SetRowsOrder`, `SetColsOrder` by weights). Built, but not tested; see the status note below. |

## Features

- **Resumable enumeration.** `Runc::RuncGlobalState` is iterative, not recursive: it keeps an
  explicit stack of local states and advances one step at a time. `dualize(max_count)` and
  `dualize(buffer_size, buffer)` return the next portion of covers, and the next call continues
  where the previous one stopped, so huge outputs can be consumed in pieces.
- **Callbacks.** Every algorithm derives from `DualizationBacktrackAlgorithmBase` and reports search
  tree nodes through two hooks, `InnerCallback` and `CoverCallback` (`IDualizationCallback`).
  Ready-made callbacks count nodes (`Counters`), collect covers (`CoverCollector`,
  `MostCoverCollector`), write them to files (`FileWriters.h`), weigh them and prune branches by
  weight (`Weighters.h`, `Pruning.h`). `dualization_callback(lambda)` wraps a lambda.
- **C ABI.** The shared library `librunc.so` exports `init_dualizer`, `set_input_row`,
  `enumerate_covers` and `release_dualizer` as `extern "C"` functions over RUNCM, for use from C
  or through `ctypes`. Rows may be given in any order; errors are reported as negative return codes
  (`RUNC_ERROR_*` in `RuncExecutor.h`) and no exception crosses the ABI.

## Repository layout

| Path | Contents |
|---|---|
| [`src/`](src/README.md) | All maintained sources. |
| [`src/common/`](src/common/README.md) | `dual::common`: bit containers (`bit_chunk`, `bit_vector`, `bit_matrix`), integer file IO, graph utilities. |
| [`src/dual/`](src/dual/README.md) | `dual::dual` (the algorithms and callbacks) and `dual::runc` (`librunc.so`, the C ABI). |
| [`python/`](python/README.md) | Python package `dual` (pybind11 bindings, pytest tests); built by `pyproject.toml`. |
| [`src/pyrunc/`](src/pyrunc/README.md) | Deprecated Python `ctypes` wrapper over `librunc.so`. |
| [`src/tests/`](src/tests/README.md) | doctest unit tests (`TestDual`, `TestUtils`) and their data. |
| [`cmake/`](cmake/README.md) | CMake helpers: compile options, install rules, package config. |
| [`datasets/`](datasets/README.md) | Input matrices in `.hg` format. |
| [`docs/`](docs/README.md) | Literature and the author's papers. |
| [`legacy/`](legacy/README.md) | Unmaintained, unbuilt code and data kept for reference. |

## Requirements

- A C++17 compiler (developed with GCC 11 on Linux).
- CMake 3.16 or newer; the presets in `CMakePresets.json` need 3.21. The presets use Ninja.
- Network access on the first configure when tests are enabled: doctest v2.5.3 is fetched with
  `FetchContent`. There are no other dependencies.
- For the Python package: Python 3.8+ and pip (numpy is optional, for array input).

## Build and test

With presets (`dev` = Debug, `release`, `asan` = RelWithDebInfo with AddressSanitizer and UBSan),
the build goes to `build/<preset>`:

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev                  # every doctest case is a CTest test: TestDual.<case>, TestUtils.<case>
ctest --preset dev -R DualizeRUNC   # a subset by regex
```

Without presets:

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build
```

All tests pass under the `dev`, `release` and `asan` presets. Besides the doctest cases, the
`dual_consumer` test installs the build into a scratch prefix and builds a `find_package(dual)`
project against it. More on running single tests in
[`src/tests/README.md`](src/tests/README.md).

CMake options: `DUAL_BUILD_TESTS` (ON when this is the top-level project), `DUAL_INSTALL` (ON when
top-level), `DUAL_ENABLE_SANITIZERS` (OFF), `DUAL_WARNINGS_AS_ERRORS` (OFF; the code still has many
`-Wall -Wextra` warnings), `DUAL_BUILD_PYTHON` (OFF; builds the Python extension module, see [`python/`](python/README.md)).

## Use from C++

Install the libraries, the headers and the CMake package into a prefix:

```bash
cmake --preset release
cmake --build --preset release
cmake --install build/release --prefix /path/to/prefix
```

This installs `lib/libdual.a`, `lib/libdual_common.a`, `lib/librunc.so`, the headers flat under
`include/dual/` and `lib/cmake/dual/dualConfig.cmake`. The package exports `dual::dual` (algorithms,
links `dual::common`), `dual::common` and `dual::runc`. A consumer project:

```cmake
cmake_minimum_required(VERSION 3.16)
project(app LANGUAGES CXX)

find_package(dual 0.1 REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE dual::dual)
```

```cpp
#include <iostream>

#include "Counters.h"
#include "Runc.h"

// The 3x3 example matrix. bit_matrix is move-only; set() grows it as needed, in any order.
//   1 1 0
//   0 1 1
//   1 0 1
bit_matrix example_matrix() {
    bit_matrix m;
    m.set(0, 0);
    m.set(0, 1);
    m.set(1, 1);
    m.set(1, 2);
    m.set(2, 0);
    m.set(2, 2);
    return m;
}

int main() {
    // 1. Portions: each call returns at most 2 covers and resumes where the previous call stopped.
    Runc::RuncGlobalState runc(Runc::RUNCM);  // or Runc::RUNC, Runc::OPT
    runc.setMatrix(example_matrix());         // or runc.loadMatrix("matrix.hg")
    for (auto covers = runc.dualize(2); !covers.empty(); covers = runc.dualize(2)) {
        for (auto const& cover : covers) {
            for (int j : cover)
                std::cout << j << ' ';
            std::cout << '\n';
        }
        std::cout << "--\n";
    }

    // 2. Callbacks: run to the end and count covers with the Counters callback.
    Runc::RuncGlobalState all(Runc::OPT);
    all.setMatrix(example_matrix());
    Counters counters;
    all.CoverCallback = &counters;
    all.dualize();
    std::cout << "covers: " << counters.cover << '\n';
}
```

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/prefix
cmake --build build
./build/app
```

Output:

```
1 2
1 0
--
0 2
--
covers: 3
```

A cover is a `std::vector<int>` of column indices (`Columns`) in the order the search added them,
not necessarily sorted. Headers are included by bare name (`"Runc.h"`) for now; a namespaced
`include/` layout is planned in #12.

## Use from Python

The Python package `dual` ([`python/`](python/README.md)) wraps the algorithms with pybind11 and is
built with scikit-build-core. From the repository root (needs a C++17 compiler; pip fetches CMake
and pybind11 into the build environment if needed):

```bash
pip install .            # or ".[test]" for pytest and numpy; then run `pytest`
```

```python
import dual

rows = [[0, 1], [1, 2], [0, 2]]               # row i: the column indices of its ones
for cover in dual.enumerate_covers(rows):     # a generator; RUNCM by default
    print(cover)                              # [1, 2], [0, 1], [0, 2]

dual.count_covers("datasets/15_20_275.hg")    # 275; a .hg path or a 2-D numpy array work too
dual.minimal_covers(rows, algorithm="opt")    # all covers as a list
```

Covers are fetched from C++ in portions with the GIL released; the RUNC-family algorithms resume
the search between portions, so a generator can be stopped at any time. See
[`python/README.md`](python/README.md) for the whole API and the development workflow. The older
`ctypes` wrapper [`src/pyrunc`](src/pyrunc/README.md) over `librunc.so` is deprecated.

## Input format

Matrices are read from `.hg` text files (see [`datasets/`](datasets/README.md)): one matrix row per
line, the indices of the columns holding a one, separated by spaces or tabs. Column indices start
at 0. The example matrix above is

```
0 1
1 2
0 2
```

Test and sample files are named `rows_cols_count.hg`, where `count` is the number of minimal covers;
for example `15_20_275.hg` has 15 rows, 20 columns and 275 minimal covers. The tests assert this
count.

## References

The algorithms and related literature are listed in [`docs/references.md`](docs/references.md); the
RUNC family is described in E. V. Djukova, P. A. Prokofjev, *Construction and investigation of new
asymptotically optimal algorithms for dualization* (2014), included in `docs/references/`.

## License

MIT, see [`LICENSE`](LICENSE). Third-party code under `legacy/3rd-party/` keeps its authors' terms.

## Status

This is research code being turned into a reusable library (epic #7). The RUNC family
(`Runc::RuncGlobalState`), the Python package `dual` and the C ABI are the main maintained part. `AOT1`, `PUNC` and
`RuncFixed` are tested against RUNCM on the test matrices and on random ones, but are less used.
The API and the header layout are expected to change (#12).
