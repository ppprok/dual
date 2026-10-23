# pybind11 extension module dual._dual (DUAL_BUILD_PYTHON=ON).
#
# Normally built by scikit-build-core through `pip install .` (see pyproject.toml), which provides
# pybind11 from the build environment and installs the module into the `dual` package of the wheel.
# A plain CMake build needs pybind11 on CMAKE_PREFIX_PATH, e.g.
# `-Dpybind11_DIR=$(python3 -m pybind11 --cmakedir)`.

set(PYBIND11_FINDPYTHON ON)
find_package(Python 3.8 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 2.12 CONFIG REQUIRED)

pybind11_add_module(_dual src/_dual.cpp)
target_link_libraries(_dual PRIVATE dual::dual)
dual_target_options(_dual)

if(SKBUILD)
    # scikit-build-core sets the install prefix to the wheel's platlib directory.
    install(TARGETS _dual LIBRARY DESTINATION dual COMPONENT python)
else()
    # Outside pip: assemble the package in <build>/python/dual (module + Python sources), so that
    # PYTHONPATH=<build>/python works without installing (see python/README.md).
    set_target_properties(_dual PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/dual")
    file(GLOB _dual_py_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/dual/*")
    add_custom_command(TARGET _dual POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${_dual_py_files} "${CMAKE_CURRENT_BINARY_DIR}/dual/"
            COMMENT "Copying the dual Python package next to _dual"
            VERBATIM)
endif()
