# src/common

Low-level building blocks shared by the algorithms: bit containers, reading and writing integer
files, small container and graph utilities. Built as the static library `dual::common`
(file `libdual_common.a`, compiled with `-fPIC` because it is linked into `librunc.so`).
Only the files listed in [`CMakeLists.txt`](CMakeLists.txt) are built and installed.

## Bit containers

| File | Contents |
|---|---|
| `chunk.h` | `chunk<T>`: a non-owning `[begin, end)` pointer range. |
| `bit_chunk.h` | `bit_chunk`: a non-owning view of a bit string stored in `unsigned int` blocks, with set/get, find, count and bitwise `assign_*` operations; `bit_chunks` is a vector of them. |
| `bit_vector.h` | `bit_vector`: a `bit_chunk` that owns its storage. |
| `bit_mask.h` | `bit_mask`: a `bit_chunk` used as a mask on other bit strings (`trim`, masked counts and searches). |
| `bit_matrix.h` | `bit_matrix`: rows as `bit_chunk`s over one contiguous block buffer. Move-only. `set(i, j)` grows the matrix in any order (missing rows are added as zero rows, rows are re-laid out when a column does not fit); it returns `false` only for negative or `INT_MAX` indices and throws `std::length_error` when the size does not fit into `int`. |
| `bits.h`, `bits.cpp` | `bits32` / `bits64` words and byte lookup tables for bit counts and lowest/highest set bits; used by `bit_chunk` and `RuncFixed`. |
| `blob.h`, `raw_vector.h` | Containers that do not run element constructors. |

## IO

| File | Contents |
|---|---|
| `IntegersFileReader.h/.cpp` | Reads integers from a text file; `ReadBitMatrix()` reads a `.hg` file into a `bit_matrix` (see the input format in the [top-level README](../../README.md#input-format)), `ReadBitMatrix32/64()` into machine words. |
| `IntegersFileWriter.h/.cpp` | Writes integers, integer matrices and the set bits of a `bit_chunk` to a text file. |
| `io_utils.h/.cpp` | `FILE*` helpers: open, read all, write. |

## Utilities

| File | Contents |
|---|---|
| `graph_algorithms.h/.cpp` | Graphs as adjacency `bit_matrix`: transitive closure, topological order, edge count. |
| `containers.h`, `range.h`, `marked.h`, `IteratorRange.h` | Container helpers, an index range, a value with an attached mark, iterator ranges. |
| `string_utils.h`, `math_utils.h`, `Randomizer.h` | String comparison helpers, `sqr`, random sampling helpers. |

## Not built or not installed

- `CSVLoader.h` / `CSVLoader.cpp`: CSV loader. The `.cpp` is not compiled and the header is not
  installed; the header is only used by `TestUtils` (`CSVTests.cpp`).
- `Registrator.h`: a name-to-instance / name-to-factory registry. Header-only, not installed; only
  used by `TestUtils` (`RegistratorTests.cpp`).

## Use

Link `dual::common` (it comes with `dual::dual`) and include headers by bare name, e.g.
`#include "bit_matrix.h"`. The tests in [`../tests/UtilsLib.Test`](../tests/README.md) exercise this
library.
