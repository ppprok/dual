# dual (Python package)

Python bindings of the [dual](https://github.com/ppprok/dual) C++ library: enumeration of all
**minimal covers of a Boolean matrix**, equivalently the minimal transversals (hitting sets) of a
hypergraph, with the RUNC family of dualization algorithms. Built with
[pybind11](https://pybind11.readthedocs.io) and
[scikit-build-core](https://scikit-build-core.readthedocs.io).

## Install

From a checkout of the repository (a C++17 compiler is needed; CMake and pybind11 are fetched
into the build environment by pip if missing):

```bash
pip install .            # or: pip install ".[test]" to also get pytest and numpy
```

numpy is optional; it is only needed to pass matrices as arrays.

## Usage

```python
import dual

rows = [[0, 1], [1, 2], [0, 2]]          # row i: the column indices of its ones

for cover in dual.enumerate_covers(rows):  # generator, covers are fetched in portions
    print(cover)                           # [1, 2], [0, 1], [0, 2] (order depends on the algorithm)

dual.minimal_covers(rows)                  # all covers as a list
dual.count_covers("src/tests/data/15_20_275.hg")   # 275, a .hg file path works too

import numpy as np
dense = np.array([[1, 1, 0], [0, 1, 1], [1, 0, 1]], dtype=bool)
dual.minimal_covers(dense, algorithm="opt")         # a 2-D array: nonzero entries are ones

for batch in dual.iter_batches(rows, batch_size=1000):   # lists of up to 1000 covers
    ...
```

A matrix is a sequence of rows of column indices, a 2-D array (anything with `__array__`), or the
path of a `.hg` file (one row per line, space-separated 0-based column indices). A matrix without
rows has one minimal cover, the empty one (`[[]]`); a matrix with an all-zero row has none (`[]`).
Each cover is a list of column indices in increasing order.

| Function | Returns |
|---|---|
| `enumerate_covers(matrix, algorithm="runcm", batch_size=1024)` | generator of covers |
| `iter_batches(matrix, algorithm="runcm", batch_size=1024)` | generator of lists of covers |
| `minimal_covers(matrix, algorithm="runcm")` | list of all covers |
| `count_covers(matrix, algorithm="runcm")` | number of covers (not materialized in Python) |
| `make_dualizer(matrix, algorithm="runcm")` | a `Dualizer` with the matrix set |
| `ALGORITHMS` | names of the algorithms |

Algorithms: `"runcm"` (default), `"runc"`, `"opt"` — the RUNC family, resumable: the search is
paused between portions, so memory does not grow with the number of covers and stopping a
generator early stops the search. `"ao1"`, `"ao2"`, `"ao1k"`, `"ao2k"`, `"ao1m"`, `"ao2m"` (AOT1)
and `"punc"` (PUNC) have no resumable mode: they compute all covers in C++ on the first portion.

The low-level class `dual.Dualizer(algorithm)` has `set_rows(rows)`, `set_dense(buffer)`,
`load_hg(path)`, `next_batch(max_count)` (an empty list means the end), `count()`, `reset()` and
the properties `n_rows`, `n_cols`, `finished`, `resumable`, `algorithm`.

The C++ search runs with the GIL released, so different `Dualizer` objects (and generators) can
work in parallel threads. One `Dualizer` must not be used by two threads at once; doing so raises
`RuntimeError`.

## Development

```bash
python3 -m venv .venv && . .venv/bin/activate
pip install ".[test]"
pytest                                  # python/tests, from the repository root
```

For iterating on the C++ side, an editable install rebuilds the extension on import:

```bash
pip install scikit-build-core pybind11 pytest numpy
pip install --no-build-isolation -Ceditable.rebuild=true -Cbuild-dir=build/py -e .
pytest
```

Without pip, the CMake option `DUAL_BUILD_PYTHON=ON` builds the module and assembles the package
in `<build>/python/dual`:

```bash
cmake -S . -B build/pyext -DDUAL_BUILD_PYTHON=ON -Dpybind11_DIR="$(python3 -m pybind11 --cmakedir)"
cmake --build build/pyext --target _dual
PYTHONPATH=build/pyext/python python3 -c "import dual; print(dual.count_covers([[0, 1], [1, 2]]))"
```

| File | Contents |
|---|---|
| `dual/__init__.py` | The public Python API. |
| `dual/_dual.pyi`, `dual/py.typed` | Type stubs of the extension module; the package is typed. |
| `src/_dual.cpp` | The pybind11 module `dual._dual` (all C++ includes of the bindings are here). |
| `CMakeLists.txt` | Builds `_dual` when `DUAL_BUILD_PYTHON=ON`; the build is driven by `../pyproject.toml`. |
| `tests/` | pytest tests. |
