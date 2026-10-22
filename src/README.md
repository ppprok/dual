# src

All maintained implementation sources. [`CMakeLists.txt`](CMakeLists.txt) adds `common` and `dual`;
the root `CMakeLists.txt` adds `tests` when `DUAL_BUILD_TESTS` is ON. The public headers are in
[`../include/dual/`](../include/dual) and are installed by
[`../cmake/DualInstall.cmake`](../cmake/README.md).

| Directory | Contents | Target |
|---|---|---|
| [`common/`](common/README.md) | Bit containers, integer file IO, graph and container utilities. | `dual::common` (`libdual_common.a`) |
| [`dual/`](dual/README.md) | Dualization algorithms (RUNC family, RuncFixed, AOT1, PUNC), callbacks, the C ABI. | `dual::dual` (`libdual.a`), `dual::runc` (`librunc.so`) |
| [`pyrunc/`](pyrunc/README.md) | Python `ctypes` wrapper over `librunc.so`. | filled by `InstallRuncLib` |
| [`tests/`](tests/README.md) | doctest unit tests and test data. | `TestDual`, `TestUtils` |

`dual::dual` links `dual::common`; `librunc.so` links `dual::dual` statically. Only the files listed
in each `CMakeLists.txt` are compiled: a new `.cpp` or `.h` has to be added there.

Layout rules:

- A header that users of the library need (directly or through another public header) is public:
  it goes to `include/dual/<snake_case>.h`, is listed in the `DUAL_*_PUBLIC_HEADERS` of its target
  and is included everywhere as `#include <dual/<name>.h>`. Everything in `include/dual/` is
  installed and documented by Doxygen.
- Any other header is internal: it stays next to its sources in `src/common` or `src/dual`, is
  included by the sources of its directory as `"<name>.h"` and by the tests as
  `"common/<name>.h"`, and is not installed.
- All C++ code is in namespace `dual` (algorithm state in `dual::Runc`, `dual::AOT1`,
  `dual::PUNC`); only the C ABI of `librunc` (`<dual/runc_executor.h>`) is outside it.

Code that is no longer built lives in [`../legacy/`](../legacy/README.md).
