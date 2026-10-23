# src/dual

The dualization algorithms, the callbacks they report to, and the C ABI. The Python package `dual`
([`../../python`](../../python/README.md)) binds these classes directly with pybind11. Only the files
listed in [`CMakeLists.txt`](CMakeLists.txt) are built. Two targets:

- `dual::dual`: static library `libdual.a` with everything below except `runc_executor.cpp`. Links
  `dual::common`.
- `dual::runc`: shared library `librunc.so` built from `runc_executor.cpp`, the `extern "C"` API.
  The `InstallRuncLib` target copies it into the deprecated [`../pyrunc`](../pyrunc/README.md).

The public headers are in [`../../include/dual/`](../../include/dual) (included as `<dual/...>`,
installed); this directory holds the sources and one internal header,
`bool_matrix_preparation.h`. All C++ code is in namespace `dual`; the algorithms keep their state in
nested namespaces named after them: `dual::Runc`, `dual::AOT1`, `dual::PUNC`, `dual::FK`. The C ABI is the only
part outside the namespace.

## Framework

| Header / source | Contents |
|---|---|
| `<dual/dualization_backtrack_algorithm_base.h>`, `dualization_backtrack_algorithm_base.cpp` | Base class of the backtracking algorithms: holds the input `bit_matrix` (`setMatrix`, read-only `get_matrix`, `mutable_matrix` which counts as a matrix change, `loadMatrix(file)`), the current search tree node (`DualizationNode`: columns, node type, weight, `stoped`/`pruned` flags) and the hooks `InnerCallback` / `CoverCallback` (`IDualizationCallback*`, not owned). Also `dualization_callback(lambda)` and `Columns` (`std::vector<int>`). |
| `<dual/counters.h>` | `Counters`: callback that counts cover, inner and extra nodes. |
| `<dual/dualization_counters.h>`, `dualization_counters.cpp` | Counters of results, inner nodes, extra steps and maximal delay. |
| `<dual/cover_collector.h>`, `cover_collector.cpp`, `<dual/weighted_cover.h>` | `CoverCollector`: callback that stores the covers (`WeightedCover`). |
| `<dual/most_cover_collector.h>`, `most_cover_collector.cpp` | `MostCoverCollector` keeps the N best covers by weight; `BitCoverCollectorCallback` stores covers as a `bit_matrix`. |
| `<dual/file_writers.h>`, `file_writers.cpp` | Callbacks that write covers (all columns, sorted, first/last/min/max column, size, weight) to a file. |
| `<dual/weighters.h>`, `weighters.cpp` | `WeighterBase` and weighters of search tree nodes by column weights or by covered row weights, with pruning by weight bounds. |
| `<dual/pruning.h>`, `pruning.cpp` | `CoveredRowsWeighter`: pruning by bounds on the total weight of covered rows. Tested together with `WeightBoundsPruningCallback` on RUNC-M (`PruningTests.cpp`). |
| `<dual/weight_bounds_pruning.h>`, `weight_bounds_pruning.cpp` | `WeightBoundsPruningCallback`: pruning by bounds on the total column weight (fixed bounds, or dynamic ones that search for the lightest / heaviest covers). `WeightCheckpoints`: weighters save their state on the way down and restore it exactly on backtrack, so `float` weights do not drift. |
| `<dual/optimal_covers.h>`, `optimal_covers.cpp` | All optimal minimal covers (ties included) by enumeration with branch pruning: `OptimalCoverCollector` and the convenience functions `optimalCovers` (column weights or length) and `optimalCoversByCoveredRows` (weight of hit target rows), for RUNC/RUNCM/OPT/AOT1/PUNC (pruning) and FK (filtering). See [`docs/optimal-covers.md`](../../docs/optimal-covers.md). |
| `bool_matrix_preparation.h/.cpp` (internal) | Removal of dependent rows (rows that contain another row). |

## Algorithms

| Header / source | Contents |
|---|---|
| `<dual/runc.h>`, `runc.cpp` | `Runc::RuncGlobalState`, the RUNC family; `Runc::AlgorithmType` = `OPT`, `RUNC`, `RUNCM` (default). Iterative: a stack of `RuncLocalState` (uncovered rows, support rows, remaining columns), `startDualize()` + `doStep()`. Public entry points: `dualize()` runs to the end reporting through the callbacks; `dualize(max_count)` returns the next at most `max_count` covers as `std::vector<Columns>`; `dualize(buffer_size, buffer)` writes `[size, col...]` records into a `uint32_t` buffer while at least `width + 1` entries are free and returns how many covers it wrote. Repeated calls of the last two resume the enumeration; `isDualizeFinished()` tells a finished enumeration from a too small buffer, `resetDualize()` restarts it, and changing the matrix resets it. An empty matrix has one (empty) cover, a zero row none; a callback that stops the search finishes the enumeration. Tested. |
| `<dual/runc_fixed.h>`, `runc_fixed.cpp` | `RuncFixed<Row>`: recursive RUNC-M for at most 32 / 64 columns (`Row` = `bits32` / `bits64`), plus `DualizeIntervals`; helpers `RuncFixedDualize<T>()`. `RuncFixed<bits32>`, `RuncFixed<bits64>` and `RuncFixedDualize<bits32/bits64>()` are explicitly instantiated in `runc_fixed.cpp` (the header only declares `RuncFixedDualize`), so only these two row types link. Tested by `TestRuncFixedO2` and the `dual_consumer` install test. |
| `<dual/aot1.h>`, `aot1.cpp` | `AOT1::GlobalState`, variants `AO1`, `AO2`, `AO1K`, `AO2K`, `AO1M` (default), `AO2M`; entry point `dualize()`. Tested against RUNCM. |
| `<dual/punc.h>`, `punc.cpp` | `PUNC::GlobalState`: enumeration with row/column order set by weights (`SetRowsOrder`, `SetColsOrder`); entry point `dualize()`. Tested against RUNCM. |
| `<dual/fk.h>`, `fk.cpp` | Khachiyan's incremental dualization. `FK::isDual(h, g)`: Fredman–Khachiyan duality test (algorithm B) of two families given as matrix rows; returns `Duality::Dual` or a witness (`NewTransversal`: a transversal of `h` with no set of `g` in it; `NotTransversal`: a set of `g` misses a set of `h`). `FK::GlobalState`: `next()` adds one minimal cover to `getCovers()` (one duality test from the root per call) or returns `false` when the list is complete; `dualize()` finds all covers in one run of the test, adding each new cover to the families on the recursion path and continuing there (Boros, Elbassioni, Gurvich, Khachiyan). Covers go to `CoverCallback` in increasing column order; no inner nodes, no restrictions. Same corner cases as RUNC. Tested against the file names, RUNCM and brute force. |

## C ABI

`<dual/runc_executor.h>` (a C header, also usable from C) and `runc_executor.cpp`, built into
`librunc.so`:

```c
#define RUNC_OK 0
#define RUNC_ERROR_INVALID_ARGUMENT (-1)
#define RUNC_ERROR_BUFFER_TOO_SMALL (-2)
#define RUNC_ERROR_INTERNAL (-3)

typedef void* DualizerHandler;
DualizerHandler init_dualizer(void);                 // new RuncGlobalState (RUNCM); NULL if out of memory
void release_dualizer(DualizerHandler dualizer);     // NULL is ignored
int32_t set_input_row(DualizerHandler dualizer, int32_t row_num, int32_t ones_num, const int32_t* ones);
int32_t enumerate_covers(DualizerHandler dualizer, uint32_t buffer_size, uint32_t* covers);
```

`set_input_row` adds the ones of row `row_num` (0-based); rows may come in any order, rows never
given are zero rows, and changing the input restarts the enumeration. `enumerate_covers` fills
the buffer with `[size, col...]` records and returns how many covers it wrote; repeated calls
continue the enumeration and 0 means it is finished. Errors are negative codes: an invalid
argument, a buffer with fewer than `width + 1` entries (nothing is written, the enumeration does
not advance) or an internal error. No exception crosses the ABI. An empty matrix has one empty
cover, a matrix with a zero row has none. The full contract is in `<dual/runc_executor.h>`.

## Use

`target_link_libraries(app PRIVATE dual::dual)` and `#include <dual/runc.h>`; see the C++ example in
the [top-level README](../../README.md#use-from-c). Tests: [`../tests`](../tests/README.md).
