cmake_minimum_required(VERSION 3.16...3.31)

# compile_commands.json for clangd (see .clangd); must be cached before project() to take effect.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Write compile_commands.json (used by clangd)")
endif()

project(dual
        VERSION 0.1.0
        DESCRIPTION "Dualization algorithms: enumeration of minimal covers of Boolean matrices (RUNC family)"
        LANGUAGES CXX)

# PROJECT_IS_TOP_LEVEL needs CMake 3.21; compute it by hand for 3.16.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(DUAL_IS_TOP_LEVEL ON)
else()
    set(DUAL_IS_TOP_LEVEL OFF)
endif()

option(DUAL_BUILD_TESTS "Build the doctest-based unit tests" ${DUAL_IS_TOP_LEVEL})
option(DUAL_BUILD_PYTHON "Build Python bindings (placeholder, see #13; librunc.so is always built)" OFF)
option(DUAL_ENABLE_SANITIZERS "Build own targets with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(DUAL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors for own targets" OFF)
option(DUAL_INSTALL "Generate install rules and the dualConfig.cmake package" ${DUAL_IS_TOP_LEVEL})
option(DUAL_BUILD_DOCS "Add the dual_docs target that builds the API documentation with Doxygen" OFF)

if(DUAL_IS_TOP_LEVEL)
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
    endif()
endif()

if(DUAL_BUILD_PYTHON)
    message(WARNING "DUAL_BUILD_PYTHON is a placeholder until #13; use the InstallRuncLib target for src/pyrunc.")
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)
include(DualTargetOptions)

add_subdirectory(src)

if(DUAL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(src/tests)
endif()

if(DUAL_INSTALL)
    include(DualInstall)
endif()

if(DUAL_BUILD_DOCS)
    include(DualDocs)
endif()
