---
name: python-bindings
description: The Python package `dual` (pybind11 + scikit-build-core, `python/`, `pyproject.toml`) — building, installing in dev mode, running pytest, changing the bindings, pitfalls. Triggers — "pip install", "Python package", "Python bindings", "pybind11", "scikit-build", "run pytest", "python tests", "import dual fails", "_dual", "add a function to the Python API", "wheel", "editable install".
---

# Python package `dual`

| Path | What |
|---|---|
| `pyproject.toml` (root) | scikit-build-core build: sets `DUAL_BUILD_PYTHON=ON`, `DUAL_BUILD_TESTS=OFF`, `DUAL_INSTALL=OFF`, builds only the `_dual` target and installs only the `python` component. Version is read from `project(dual VERSION ...)` in `CMakeLists.txt`. |
| `python/CMakeLists.txt` | `find_package(Python ... Development.Module)` + `find_package(pybind11 CONFIG)`, `pybind11_add_module(_dual ...)` linking `dual::dual`. Under pip (`SKBUILD`) installs into `dual/`; a plain CMake build assembles the package in `<build>/python/dual`. |
| `python/src/_dual.cpp` | The whole binding: class `Dualizer`, `ALGORITHMS`, `_live_dualizers()`. **All C++ library includes and API uses are in this file** (top block "Library API used by the bindings") — the only place to adapt when the public headers change (#12: `<dual/...>` paths, namespace `dual`). |
| `python/dual/__init__.py` | Public API: `enumerate_covers`, `iter_batches`, `minimal_covers`, `count_covers`, `make_dualizer`; input dispatch (rows / array / `.hg` path). |
| `python/dual/_dual.pyi`, `py.typed` | Stubs of the extension module. Keep them in sync with `_dual.cpp` by hand. |
| `python/tests/test_dual.py` | pytest: `.hg` counts from `src/tests/data`, brute force on random matrices for every algorithm, edge cases, batches == full, generator leak check, threads. |

## Build, install, test

The dev machine has no `ensurepip` (`python3 -m venv` fails): create venvs with `uv venv --seed`
(add `--python 3.10` for the system Python; otherwise uv may pick its own newer one).

```bash
uv venv --seed .venv && . .venv/bin/activate      # or python3 -m venv .venv where it works
pip install ".[test]"                              # isolated build: fetches scikit-build-core, pybind11 (and cmake if too old)
pytest                                             # from the repo root; testpaths = python/tests
```

Iterating on C++ — editable install without build isolation; the module is rebuilt on `import dual`
when a source changed (the build dir `build/py` is kept, so rebuilds are incremental):

```bash
pip install scikit-build-core pybind11 pytest numpy
pip install --no-build-isolation -Ceditable.rebuild=true -Ceditable.verbose=false -Cbuild-dir=build/py -e .
pytest
```

Without pip (e.g. to see compiler errors in full or for clangd):

```bash
cmake -S . -B build/pyext -G Ninja -DDUAL_BUILD_PYTHON=ON -DDUAL_BUILD_TESTS=OFF \
      -DPython_EXECUTABLE=$(which python) -Dpybind11_DIR=$(python -m pybind11 --cmakedir)
cmake --build build/pyext --target _dual
PYTHONPATH=build/pyext/python pytest
```

The C++ side is still checked with `ctest --preset dev` (see `build-and-test`); `DUAL_BUILD_PYTHON` is
OFF in the presets, so ctest does not need pybind11. Run pytest too when `src/dual` or `src/common`
change: it compares all algorithms (also AOT1 and PUNC, which the doctest suite covers less) with
brute force.

## Design rules

- `Dualizer` owns the algorithm through `std::unique_ptr` (algorithms are non-copyable,
  non-movable); the Python object is held by pybind11's default `unique_ptr` holder.
- RUNC family (`runcm`, `runc`, `opt`) is resumable: `next_batch` calls
  `RuncGlobalState::dualize(max_count)`. AOT1/PUNC have no resumable mode: the first `next_batch`
  runs the full `dualize()` with a collecting `CoverCallback` and then hands covers out.
- Long C++ calls run under `py::gil_scoped_release`; never touch Python objects inside that
  scope (convert after it). A per-object mutex is taken with `try_lock` *before* releasing the GIL:
  concurrent use of one `Dualizer` raises `RuntimeError` instead of deadlocking or corrupting state.
- Covers are returned sorted (the C++ order is the search order). Input semantics follow the C++
  contract: no rows → `[[]]`, a zero row → `[]`.
- numpy is optional: arrays are converted in Python (`asarray(m) != 0`, C-contiguous bool) and
  passed through the buffer protocol (`set_dense`), so the extension never imports numpy.
- `count()` runs a full enumeration with a counting callback and restarts the portion state.

## Pitfalls

- Do not add `__init__.py` to `python/tests`: pytest would then put `python/` on `sys.path` and
  import the source `python/dual` (which has no compiled `_dual`) instead of the installed package.
- `pip install .` copies the tree to a temporary build dir each time and recompiles everything; use
  the editable command above for iteration.
- With `editable.rebuild=true`, an import after a C++ change triggers a rebuild; compile errors show
  up as an `ImportError` at import time. `editable.verbose=false` hides the (noisy, pre-existing)
  `-Wparentheses` warnings from `bit_chunk.h` on every rebuild.
- In the non-resumable path, never compute `collected_.size() - next_` after freeing the vector
  without resetting `next_` (it was an unsigned underflow → segfault on the call after the last one).
- `test_interrupted_generator_does_not_leak` relies on `_dual._live_dualizers()`; keep the counter
  in the `Dualizer` constructor/destructor if you refactor.
- The name `dual` is taken on PyPI (an unrelated project); publishing would need another
  distribution name (the import name can stay `dual`).
- The ctypes wrapper `src/pyrunc` is deprecated; the C ABI `librunc.so` stays supported.
