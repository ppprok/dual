# src/dual

The dualization algorithms, the callbacks they report to, and the C ABI for Python. Only the files
listed in [`CMakeLists.txt`](CMakeLists.txt) are built. Two targets:

- `dual::dual`: static library `libdual.a` with everything below except `RuncExecutor.cpp`. Links
  `dual::common`.
- `dual::runc`: shared library `librunc.so` built from `RuncExecutor.cpp`, the `extern "C"` API.
  The `InstallRuncLib` target copies it into [`../pyrunc`](../pyrunc/README.md).

## Framework

| File | Contents |
|---|---|
| `DualizationBacktrackAlgorithmBase.h/.cpp` | Base class of the backtracking algorithms: holds the input `bit_matrix` (`setMatrix`, `get_matrix`, `loadMatrix(file)`), the current search tree node (`DualizationNode`: columns, node type, weight, `stoped`/`pruned` flags) and the hooks `InnerCallback` / `CoverCallback` (`IDualizationCallback*`, not owned). Also `dualization_callback(lambda)` and `Columns` (`std::vector<int>`). |
| `Counters.h` | `Counters`: callback that counts cover, inner and extra nodes. |
| `DualizationCounters.h/.cpp` | Counters of results, inner nodes, extra steps and maximal delay. |
| `CoverCollector.h/.cpp`, `WeightedCover.h` | `CoverCollector`: callback that stores the covers (`WeightedCover`). |
| `MostCoverCollector.h/.cpp` | `MostCoverCollector` keeps the N best covers by weight; `BitCoverCollectorCallback` stores covers as a `bit_matrix`. |
| `FileWriters.h/.cpp` | Callbacks that write covers (all columns, sorted, first/last/min/max column, size, weight) to a file. |
| `Weighters.h/.cpp` | `WeighterBase` and weighters of search tree nodes by column weights or by covered row weights, with pruning by weight bounds. |
| `Pruning.h/.cpp` | `CoveredRowsWeighter`: pruning by bounds on the total weight of covered rows. Tested together with `WeightBoundsPruningCallback` on RUNC-M (`PruningTests.cpp`). |
| `WeightBoundsPruning.h/.cpp` | `WeightBoundsPruningCallback`: pruning by bounds on the total column weight (fixed bounds, or dynamic ones that search for the lightest / heaviest covers). |
| `BoolMatrixPreparation.h/.cpp` | Removal of dependent rows (rows that contain another row). |

## Algorithms

| File | Contents |
|---|---|
| `Runc.h/.cpp` | `Runc::RuncGlobalState`, the RUNC family; `Runc::AlgorithmType` = `OPT`, `RUNC`, `RUNCM` (default). Iterative: a stack of `RuncLocalState` (uncovered rows, support rows, remaining columns), `startDualize()` + `doStep()`. Public entry points: `dualize()` runs to the end reporting through the callbacks; `dualize(max_count)` returns the next at most `max_count` covers as `std::vector<Columns>`; `dualize(buffer_size, buffer)` writes `[size, col...]` records into a `uint32_t` buffer while at least `width + 1` entries are free and returns how many covers it wrote. Repeated calls of the last two resume the enumeration; `isDualizeFinished()` tells a finished enumeration from a too small buffer, `resetDualize()` restarts it, and changing the matrix resets it. An empty matrix has one (empty) cover, a zero row none; a callback that stops the search finishes the enumeration. Tested. |
| `RuncFixed.h/.cpp` | `RuncFixed<Row>`: recursive RUNC-M for at most 32 / 64 columns (`Row` = `bits32` / `bits64`), plus `DualizeIntervals`; helpers `RuncFixedDualize<T>()`. `RuncFixed<bits32>`, `RuncFixed<bits64>` and `RuncFixedDualize<bits32/bits64>()` are explicitly instantiated in `RuncFixed.cpp` (the header only declares `RuncFixedDualize`), so only these two row types link. Tested by `TestRuncFixedO2` and the `dual_consumer` install test. |
| `AOT1.h/.cpp` | `AOT1::GlobalState`, variants `AO1`, `AO2`, `AO1K`, `AO2K`, `AO1M` (default), `AO2M`; entry point `dualize()`. Tested against RUNCM. |
| `PUNC.h/.cpp` | `PUNC::GlobalState`: enumeration with row/column order set by weights (`SetRowsOrder`, `SetColsOrder`); entry point `dualize()`. Tested against RUNCM. |

## C ABI

`RuncExecutor.h/.cpp`, built into `librunc.so`:

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
cover, a matrix with a zero row has none. The full contract is in `RuncExecutor.h`.

## Use

`target_link_libraries(app PRIVATE dual::dual)` and `#include "Runc.h"`; see the C++ example in
the [top-level README](../../README.md#use-from-c). Tests: [`../tests`](../tests/README.md).
