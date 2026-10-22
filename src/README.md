# src

All maintained sources. [`CMakeLists.txt`](CMakeLists.txt) adds `common` and `dual` and passes their
header lists up to the install rules in [`../cmake/DualInstall.cmake`](../cmake/README.md); the root
`CMakeLists.txt` adds `tests` when `DUAL_BUILD_TESTS` is ON.

| Directory | Contents | Target |
|---|---|---|
| [`common/`](common/README.md) | Bit containers, integer file IO, graph and container utilities. | `dual::common` (`libdual_common.a`) |
| [`dual/`](dual/README.md) | Dualization algorithms (RUNC family, RuncFixed, AOT1, PUNC), callbacks, the C ABI. | `dual::dual` (`libdual.a`), `dual::runc` (`librunc.so`) |
| [`pyrunc/`](pyrunc/README.md) | Deprecated Python `ctypes` wrapper over `librunc.so` (use the `dual` package in [`../python`](../python/README.md)). | filled by `InstallRuncLib` |
| [`tests/`](tests/README.md) | doctest unit tests and test data. | `TestDual`, `TestUtils` |

`dual::dual` links `dual::common`; `librunc.so` links `dual::dual` statically. Only the files listed
in each `CMakeLists.txt` are compiled: a new `.cpp` or `.h` has to be added there. Headers include
each other by bare name (`"bit_matrix.h"`), and are installed flat into `include/dual/`. Code that
is no longer built lives in [`../legacy/`](../legacy/README.md).
