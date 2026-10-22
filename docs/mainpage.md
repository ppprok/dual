# dual: dualization algorithms {#mainpage}

C++17 library that enumerates all **minimal covers** of a Boolean matrix. A cover is a set of
columns with a one in every row; it is minimal if no proper subset is a cover. Equivalently, the
library enumerates the minimal transversals of a hypergraph (rows are edges, columns are vertices).
The algorithms are described in the papers listed in `docs/references.md`.

## Modules

| CMake target | Contents |
|---|---|
| `dual::common` (`libdual_common.a`) | Bit containers and IO: bit_chunk (non-owning bit set view), bit_vector (owning bit array), bit_mask (operations under a mask), bit_matrix (growable bit matrix), raw_vector / blob, IntegersFileReader / IntegersFileWriter (text I/O, `.hg` files). |
| `dual::dual` (`libdual.a`) | Dualization algorithms and callbacks: Runc::RuncGlobalState (RUNC, RUNCM, OPT; resumable), AOT1::GlobalState (AO1/AO2 variants), PUNC::GlobalState, RuncFixed (32/64 columns); cover collectors (CoverCollector, MostCoverCollector, BitCoverCollectorCallback), file writers, Counters, weighters and pruning callbacks. |
| `dual::runc` (`librunc.so`) | C ABI over RUNCM for `ctypes` (`src/pyrunc/runc.py`): see runc_executor.h. |

All headers are under `include/dual/` and are included as `#include <dual/runc.h>`. Everything
except the C ABI is in namespace `dual`; the algorithms keep their state in the nested namespaces
`dual::Runc`, `dual::AOT1` and `dual::PUNC`.

## Input format (`.hg`)

A text file with one matrix row per line; a line lists the 0-based indices of the columns that hold
ones, separated by spaces. An empty line is a zero row; trailing empty lines are ignored. The
number of columns is one past the largest index. Test files are named `rows_cols_covers.hg`, e.g.
`15_20_275.hg` has 275 minimal covers. Read one with
DualizationBacktrackAlgorithmBase::loadMatrix or IntegersFileReader::ReadBitMatrix.

Edge cases: a matrix without rows has exactly one minimal cover, the empty set; a matrix with a
zero row has none.

## Algorithms and callbacks

Every algorithm derives from DualizationBacktrackAlgorithmBase, which holds the input matrix and
two callback pointers (IDualizationCallback):

- `CoverCallback` is called with each minimal cover (DualizationNode::columns);
- `InnerCallback` is called for every new and backtrack node of the search tree.

A callback may stop the enumeration (DualizationNode::stoped) or prune a branch
(DualizationNode::pruned). Use dualization_callback() to wrap a lambda.

```cpp
#include <dual/cover_collector.h>
#include <dual/runc.h>

dual::Runc::RuncGlobalState runc(dual::Runc::RUNCM);
runc.loadMatrix("data/15_20_275.hg");
dual::CoverCollector collector;
runc.CoverCallback = &collector;
runc.dualize();                       // collector.GetCovers().size() == 275
```

## Resumable enumeration

Runc::RuncGlobalState is iterative rather than recursive, so enumeration can be paused and resumed:
Runc::RuncGlobalState::dualize(size_t) returns the next portion of covers and
Runc::RuncGlobalState::dualize(uint32_t, uint32_t*) writes them into a buffer as
`[size, col...]` records. The full contract is in the class description. The C ABI in
runc_executor.h (`init_dualizer`, `set_input_row`, `enumerate_covers`, `release_dualizer`) is built
on it.

## Building this documentation

Configure with `-DDUAL_BUILD_DOCS=ON` and build the `dual_docs` target (needs Doxygen), or run
`doxygen docs/Doxyfile` from the repository root. HTML goes to `<build dir>/docs/html`.
