# cmake

CMake modules used by the root [`CMakeLists.txt`](../CMakeLists.txt), which adds this directory to
`CMAKE_MODULE_PATH`.

| File | Contents |
|---|---|
| `DualTargetOptions.cmake` | `dual_target_options(<target>)`: C++17 without extensions, `-Wall -Wextra` (`/W4` on MSVC), `-Werror` for GCC/Clang when `DUAL_WARNINGS_AS_ERRORS` is ON (OFF by default, ON in all presets; not applied to MSVC, whose `/W4` still warns about the int-sized design), ASan + UBSan when `DUAL_ENABLE_SANITIZERS` is ON. Applied to every own target (libraries and tests), not to third-party code. |
| `DualInstall.cmake` | Install rules, included when `DUAL_INSTALL` is ON (default for a top-level build): the libraries `common`, `dual`, `runc`, the public header tree `include/dual/` (internal headers in `src/` are not installed; targets carry `$<INSTALL_INTERFACE:include>`), the export set `dualTargets` with namespace `dual::`, `dualConfig.cmake` and `dualConfigVersion.cmake` (version compatibility `SameMinorVersion`) into `lib/cmake/dual/` (cache variable `DUAL_INSTALL_CMAKEDIR`). |
| `dualConfig.cmake.in` | Template of the package config: includes `dualTargets.cmake`. |

## Check the package

```bash
cmake --preset release
cmake --build --preset release
cmake --install build/release --prefix /path/to/prefix
```

Then configure a consumer with `-DCMAKE_PREFIX_PATH=/path/to/prefix`, `find_package(dual REQUIRED)`
and `target_link_libraries(app PRIVATE dual::dual)`; a full example is in the
[top-level README](../README.md#use-from-c).
