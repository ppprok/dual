# dual (Python package)

Python bindings of the [dual](https://github.com/ppprok/dual) C++ library: enumeration of all
**minimal covers of a Boolean matrix**, equivalently the minimal transversals (hitting sets) of a
hypergraph, with the RUNC family of dualization algorithms, and the Fredman–Khachiyan duality
test. Built with
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

dual.is_dual(rows, [[0, 1], [0, 2], [1, 2]])       # None: these are exactly the minimal covers
dual.is_dual(rows, [[0, 1], [0, 2]])               # DualityWitness(kind='new_transversal', columns=[1, 2])

d = dual.make_dualizer(rows, algorithm="fk")       # Khachiyan's incremental dualization
d.next_cover()                                     # one more cover, or None when there are no more
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
| `is_dual(h, g)` | `None` if the rows of `g` are exactly the minimal covers of `h`, else a `DualityWitness` |
| `ALGORITHMS` | names of the algorithms |

Algorithms: `"runcm"` (default), `"runc"`, `"opt"` — the RUNC family, resumable: the search is
paused between portions, so memory does not grow with the number of covers and stopping a
generator early stops the search. `"ao1"`, `"ao2"`, `"ao1k"`, `"ao2k"`, `"ao1m"`, `"ao2m"` (AOT1)
and `"punc"` (PUNC) have no resumable mode: they compute all covers in C++ on the first portion.
`"fk"` is Khachiyan's incremental dualization (Boros, Elbassioni, Gurvich, Khachiyan): each next
cover is found by the Fredman–Khachiyan duality test of the covers found so far, in time
quasi-polynomial in the input and those covers. It is resumable, but every portion repeats the
test from scratch and keeps all covers found, so collecting a large enumeration from its
generators is slow; `minimal_covers` and `count_covers` run it in one pass instead (still much
slower than RUNC: about 4 s vs 0.01 s for the 7187 covers of `10_50_7187.hg`).

`is_dual(h, g)` is the Fredman–Khachiyan duality test (algorithm B). Both matrices are reduced to
their minimal rows first; `h` and `g` are dual when the minimal rows of `g` are exactly the
minimal covers of `h` (the relation is symmetric). A matrix without rows is dual only to `[[]]`,
a matrix with a zero row only to `[]`. If they are not dual, the result is a
`DualityWitness(kind, columns)`: with kind `"new_transversal"` the columns cover `h` and contain
no row of `g` (so `h` has a minimal cover missing from `g`); with `"not_transversal"` they contain
a row of `g` and their complement contains a row of `h` (so that row of `g` is not a cover).

The low-level class `dual.Dualizer(algorithm)` has `set_rows(rows)`, `set_dense(buffer)`,
`load_hg(path)`, `next_batch(max_count)` (an empty list means the end), `next_cover()` (`None`
means the end), `all_covers()` (all covers in one run), `count()`, `reset()` and
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
