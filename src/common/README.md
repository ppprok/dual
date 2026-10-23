# src/common

Low-level building blocks shared by the algorithms: bit containers, reading and writing integer
files, small container and graph utilities. Built as the static library `dual::common`
(file `libdual_common.a`, compiled with `-fPIC` because it is linked into `librunc.so`).
Only the files listed in [`CMakeLists.txt`](CMakeLists.txt) are built.

The **public** headers of this library live in [`../../include/dual/`](../../include/dual) and are
installed; they are included as `<dual/...>`. This directory holds the implementation (`.cpp`) and
the **internal** headers, which are not installed. Everything is in namespace `dual`.

## Bit containers (public)

| Header | Contents |
|---|---|
| `<dual/chunk.h>` | `chunk<T>`: a non-owning `[begin, end)` pointer range; `begin_offset`/`end_offset`/`rebase` move it to another buffer. |
| `<dual/bit_chunk.h>` | `bit_chunk`: a non-owning view of a bit string stored in `unsigned int` blocks, with set/get, find, count and bitwise `assign_*` operations; `bit_chunks` is a vector of them. |
| `<dual/bit_vector.h>` | `bit_vector`: a `bit_chunk` that owns its storage. |
| `<dual/bit_mask.h>` | `bit_mask`: a `bit_chunk` used as a mask on other bit strings (`trim`, masked counts and searches). |
| `<dual/bit_matrix.h>` | `bit_matrix`: rows as `bit_chunk`s over one contiguous block buffer. Move-only. `set(i, j)` grows the matrix in any order (missing rows are added as zero rows, rows are re-laid out when a column does not fit); it returns `false` only for negative or `INT_MAX` indices and throws `std::length_error` when the size does not fit into `int`. |
| `<dual/bits.h>`, `bits.cpp` | `bits32` / `bits64` words and byte lookup tables for bit counts and lowest/highest set bits; used by `bit_chunk` and `RuncFixed`. |
| `<dual/blob.h>`, `<dual/raw_vector.h>` | Containers that do not run element constructors; they throw `std::bad_alloc` and stay unchanged when memory runs out. |
| `<dual/containers.h>`, `<dual/range.h>`, `<dual/marked.h>` | Container helpers, an index range, a value with an attached mark (used by the algorithms' headers). |

## IO (public)

| Header / source | Contents |
|---|---|
| `<dual/integers_file_reader.h>`, `integers_file_reader.cpp` | Reads integers from a text file; `ReadBitMatrix()` reads a `.hg` file into a `bit_matrix` (see the input format in the [top-level README](../../README.md#input-format)), `ReadBitMatrix32/64()` into machine words. |
| `<dual/integers_file_writer.h>`, `integers_file_writer.cpp` | Writes integers, integer matrices and the set bits of a `bit_chunk` to a text file. |
| `<dual/io_utils.h>`, `io_utils.cpp` | `FILE*` helpers: open, read all, write. |

## Internal (this directory, not installed)

| File | Contents |
|---|---|
| `graph_algorithms.h/.cpp` | Graphs as adjacency `bit_matrix`: transitive closure, topological order, edge count. Compiled into `dual::common`, tested by `TestUtils`. |
| `iterator_range.h`, `string_utils.h`, `math_utils.h`, `randomizer.h` | Iterator ranges, string comparison helpers, `sqr`, random sampling helpers. |
| `csv_loader.h` / `csv_loader.cpp` | CSV loader. The `.cpp` is not compiled; the header is only used by `TestUtils` (`CSVTests.cpp`). |
| `registrator.h` | A name-to-instance / name-to-factory registry, only used by `TestUtils` (`RegistratorTests.cpp`). |

Tests include internal headers relative to `src/`, e.g. `#include "common/string_utils.h"`.

## Use

Link `dual::common` (it comes with `dual::dual`) and include the public headers, e.g.
`#include <dual/bit_matrix.h>`. The tests in [`../tests/UtilsLib.Test`](../tests/README.md) exercise
this library.
