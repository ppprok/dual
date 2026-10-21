# src/pyrunc

Python access to RUNCM through the C ABI of `librunc.so` (see [`../dual`](../dual/README.md#c-abi)).
Not a package yet: a pip-installable module is planned in #13.

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
  Negative indices are dropped; a row with no ones is skipped and the call returns `False`.
- `enumerate_covers(buffer_count=128)`: returns the next portion of minimal covers as a list of
  numpy `int32` arrays of column indices (in search order, not sorted). The buffer has room for
  `buffer_count` covers of the maximal size. Call it repeatedly; an empty list means the
  enumeration is finished. Add all rows before the first call.

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
