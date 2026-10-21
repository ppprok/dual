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
| `Pruning.h/.cpp` | `CoveredRowsWeighter`: pruning by bounds on the total weight of covered rows. |
| `WeightBoundsPruning.h` | `WeightBoundsPruningCallback`: pruning by bounds on the total column weight. Its `WeightBoundsPruning.cpp` is **not compiled**, so code that calls these methods (including `CoveredRowsWeighter`) fails to link. |
| `BoolMatrixPreparation.h/.cpp` | Removal of dependent rows (rows that contain another row). |

## Algorithms

| File | Contents |
|---|---|
| `Runc.h/.cpp` | `Runc::RuncGlobalState`, the RUNC family; `Runc::AlgorithmType` = `OPT`, `RUNC`, `RUNCM` (default). Iterative: a stack of `RuncLocalState` (uncovered rows, support rows, remaining columns), `startDualize()` + `doStep()`. Public entry points: `dualize()` runs to the end reporting through the callbacks; `dualize(max_count)` returns the next at most `max_count` covers as `std::vector<Columns>`; `dualize(buffer_size, buffer)` writes `[size, col...]` records into a `uint32_t` buffer while at least `width + 1` entries are free and returns how many covers it wrote. Repeated calls of the last two resume the enumeration; an empty result / 0 means it is finished. Tested. |
| `RuncFixed.h/.cpp` | `RuncFixed<Row>`: recursive RUNC-M for at most 32 / 64 columns (`Row` = `bits32` / `bits64`), plus `DualizeIntervals`; helpers `RuncFixedDualize<T>()`. Only `RuncFixedDualize` is explicitly instantiated in the library, and the header defines it inline, so a caller has to declare `extern template ... RuncFixedDualize<bits64>(...)` to link. Its tests (`../tests/DualizationLib.Test/RuncFixedTests.cpp`) are not built. |
| `AOT1.h/.cpp` | `AOT1::GlobalState`, variants `AO1`, `AO2`, `AO1K`, `AO2K`, `AO1M` (default), `AO2M`; entry point `Dualize()`. Not tested. The class does not override the base's pure virtual `dualize()`, so it is abstract: derive from it to instantiate. |
| `PUNC.h/.cpp` | `PUNC::GlobalState`: enumeration with row/column order set by weights (`SetRowsOrder`, `SetColsOrder`); entry point `Dualize()`. Not tested, abstract for the same reason as `AOT1`. |

A quick run of `AOT1` and `PUNC` on the test matrices on `master` gave cover counts that differ
from the expected ones for some `AOT1` variants, `PUNC` crashed on `15_20_275.hg` and `AO1M` on
`10_25_480.hg`; treat them as experimental.

## C ABI

`RuncExecutor.h/.cpp`, built into `librunc.so`:

```c
typedef void* DualizerHandler;
DualizerHandler init_dualizer();                     // new RuncGlobalState (RUNCM)
void release_dualizer(DualizerHandler dualizer);
void set_input_row(DualizerHandler dualizer, int32_t row_num, int32_t ones_num, int32_t* ones);
uint32_t enumerate_covers(DualizerHandler dualizer, uint32_t buffer_size, uint32_t* covers);
```

`set_input_row` sets the ones of row `row_num`; pass rows in increasing order (see `bit_matrix` in
[`../common`](../common/README.md)) and before the first `enumerate_covers`. `enumerate_covers`
calls `dualize(buffer_size, covers)`: it fills the buffer with `[size, col...]` records and returns
the number of covers written; repeated calls continue the enumeration, and 0 means it is finished
(or that the buffer has fewer than `width + 1` entries). The algorithm is always RUNCM. Errors are
not reported.

## Not built

`GeneticDualizer.h` and `FrequentInfrequentEnumerator.h` are dead leftovers of the poset code
(see [`legacy/`](../../legacy/README.md)); nothing includes them and CMake does not list them.

## Use

`target_link_libraries(app PRIVATE dual::dual)` and `#include "Runc.h"`; see the C++ example in
the [top-level README](../../README.md#use-from-c). Tests: [`../tests`](../tests/README.md).
