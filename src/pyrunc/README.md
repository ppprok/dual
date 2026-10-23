# src/pyrunc

**Deprecated:** use the pip-installable package `dual` in [`../../python`](../../python/README.md)
(`pip install .` from the repository root). Importing `runc.py` emits a `DeprecationWarning`.

Python access to RUNCM through the C ABI of `librunc.so` (see [`../dual`](../dual/README.md#c-abi)).
It is kept as a minimal example of calling the C ABI from another language, and as a quick check
of `librunc.so`, which stays a supported artifact.

| Old (`runc.py`) | New (`dual` package) |
|---|---|
| `RuncDualizer()` + `add_input_row(row)` per row | `dual.make_dualizer(rows)` or `dual.Dualizer().set_rows(rows)` |
| `enumerate_covers(buffer_count)` until it returns `[]` | `dual.iter_batches(rows, batch_size=...)`, or `Dualizer.next_batch(n)` |
| covers as numpy `int32` arrays in search order | covers as lists of `int`, sorted |

| File | Contents |
|---|---|
| `runc.py` | Loads `librunc.so` from its own directory with `ctypes` and defines `RuncDualizer`, plus example functions `test_enumerate`, `test_enumerate_large` and `test_enumerate_from_file`. Requires numpy. |
| `librunc.so` | Not in git (`*.so` is ignored); copied here by the `InstallRuncLib` CMake target. |

## Setup

From the repository root:

```bash
cmake --preset release
cmake --build --preset release --target InstallRuncLib
```

## API

- `RuncDualizer()`: creates a dualizer (`init_dualizer`); `__del__` releases it.
- `add_input_row(columns_with_ones)`: appends the next matrix row given by the indices of its ones.
  An index outside `[0, 2**31 - 1)` raises `ValueError`; an empty row is a zero row, and then the
  matrix has no covers.
- `enumerate_covers(buffer_count=128)`: returns the next portion of minimal covers as a list of
  numpy `int32` arrays of column indices (in search order, not sorted). The buffer has room for
  `buffer_count` covers of the maximal size. Call it repeatedly; an empty list means the
  enumeration is finished. Adding a row restarts the enumeration. A C ABI error raises
  `RuntimeError`; a too large `buffer_count` raises `ValueError`.

```python
import sys
sys.path.insert(0, "src/pyrunc")
from runc import RuncDualizer

runc = RuncDualizer()
with open("datasets/15_20_275.hg") as f:
    for line in f:
        runc.add_input_row([int(a) for a in line.split()])

total = 0
while True:
    covers = runc.enumerate_covers(10)
    if not covers:
        break
    total += len(covers)
print(total)  # 275
```

`test_enumerate_from_file` in `runc.py` opens a hardcoded absolute path
(`/home/user/logical/dual/src/tests/data/6_6_4.hg`); edit it before running.
