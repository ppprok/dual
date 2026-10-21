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
| **RuncFixed** | `RuncFixed<bits32>`, `RuncFixed<bits64>`, `RuncFixedDualize()` | Recursive RUNC-M for matrices of at most 32 or 64 columns, each row stored in one machine word. Also has `DualizeIntervals()`. Not covered by the built tests. |
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
  `enumerate_covers` and `release_dualizer` as `extern "C"` functions over RUNCM, used from Python
  through `ctypes`.

## Repository layout

| Path | Contents |
|---|---|
| [`src/`](src/README.md) | All maintained sources. |
| [`src/common/`](src/common/README.md) | `dual::common`: bit containers (`bit_chunk`, `bit_vector`, `bit_matrix`), integer file IO, graph utilities. |
| [`src/dual/`](src/dual/README.md) | `dual::dual` (the algorithms and callbacks) and `dual::runc` (`librunc.so`, the C ABI). |
| [`src/pyrunc/`](src/pyrunc/README.md) | Python `ctypes` wrapper over `librunc.so`. |
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
- For the Python wrapper: Python 3 and numpy.

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

Known failures on `master`: `TestUtils.load_CSV_and_create_columns` fails in every configuration,
and under the `asan` preset the RUNC tests and several `TestUtils` IO and graph tests report
heap-buffer-overflow / use-after-free. These are pre-existing defects. More on running single tests
in [`src/tests/README.md`](src/tests/README.md).

CMake options: `DUAL_BUILD_TESTS` (ON when this is the top-level project), `DUAL_INSTALL` (ON when
top-level), `DUAL_ENABLE_SANITIZERS` (OFF), `DUAL_WARNINGS_AS_ERRORS` (OFF; the code still has many
`-Wall -Wextra` warnings), `DUAL_BUILD_PYTHON` (a placeholder, does nothing yet).

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

// The 3x3 example matrix. bit_matrix is move-only; set bits row by row, in increasing row order.
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
0 1
0 2
--
1 2
--
covers: 3
```

A cover is a `std::vector<int>` of column indices (`Columns`) in the order the search added them,
not necessarily sorted. Headers are included by bare name (`"Runc.h"`) for now; a namespaced
`include/` layout is planned in #12.

## Use from Python

[`src/pyrunc/runc.py`](src/pyrunc/README.md) wraps `librunc.so` with `ctypes` (requires numpy).
Build the library and copy it next to `runc.py`:

```bash
cmake --preset release
cmake --build --preset release --target InstallRuncLib
```

```python
import sys
sys.path.insert(0, "src/pyrunc")
from runc import RuncDualizer

runc = RuncDualizer()
runc.add_input_row([0, 1])     # row 0: ones in columns 0 and 1
runc.add_input_row([1, 2])
runc.add_input_row([0, 2])
while True:
    covers = runc.enumerate_covers()   # next portion; the buffer is sized for 128 covers by default
    if not covers:
        break
    for cover in covers:
        print(sorted(cover.tolist()))  # [0, 1], [0, 2], [1, 2]
```

A pip-installable package is planned in #13.

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
(`Runc::RuncGlobalState`) and the C ABI are the maintained, tested part. `AOT1`, `PUNC` and
`RuncFixed` are built but have no tests in the current build; a quick run on `master` showed that
some `AOT1` variants report cover counts different from the expected ones and that `AOT1` and
`PUNC` can crash, so treat them as experimental. The API, the header layout and the C ABI
are expected to change.
