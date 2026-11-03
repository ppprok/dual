# Public headers of dual::dual (installed, included as <dual/...>).
set(DUAL_DUAL_PUBLIC_HEADERS
        aot1.h
        completion_bounds.h
        counters.h
        cover_collector.h
        dualization_backtrack_algorithm_base.h
        dualization_counters.h
        file_writers.h
        fk.h
        most_cover_collector.h
        optimal_covers.h
        pruning.h
        punc.h
        runc.h
        runc_fixed.h
        weight_bounds_pruning.h
        weighted_cover.h
        weighters.h)
list(TRANSFORM DUAL_DUAL_PUBLIC_HEADERS PREPEND "${PROJECT_SOURCE_DIR}/include/dual/")

# Static library with the dualization algorithms.
add_library(dual
        aot1.cpp
        bool_matrix_preparation.cpp
        completion_bounds.cpp
        cover_collector.cpp
        dualization_backtrack_algorithm_base.cpp
        dualization_counters.cpp
        file_writers.cpp
        fk.cpp
        most_cover_collector.cpp
        optimal_covers.cpp
        pruning.cpp
        punc.cpp
        runc.cpp
        runc_fixed.cpp
        weight_bounds_pruning.cpp
        weighters.cpp
        bool_matrix_preparation.h  # internal
        runc_fixed_width.h  # internal
        ${DUAL_DUAL_PUBLIC_HEADERS})
add_library(dual::dual ALIAS dual)

# The include directory comes from dual::common.
target_link_libraries(dual PUBLIC dual::common)
set_target_properties(dual PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME dual)
dual_target_options(dual)

# Shared library with the C ABI (<dual/runc_executor.h>) used by src/pyrunc/runc.py via ctypes.
find_package(Threads REQUIRED)
add_library(runc SHARED runc_executor.cpp "${PROJECT_SOURCE_DIR}/include/dual/runc_executor.h")
add_library(dual::runc ALIAS runc)
target_include_directories(runc PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(runc PRIVATE dual::dual Threads::Threads)
# The C functions carry no __declspec(dllexport); on Windows export everything so that runc.dll gets
# an import library (runc.lib), which the installed dual::runc target refers to.
set_target_properties(runc PROPERTIES EXPORT_NAME runc WINDOWS_EXPORT_ALL_SYMBOLS ON)
dual_target_options(runc)

# Copies librunc.so next to src/pyrunc/runc.py (to be replaced by DUAL_BUILD_PYTHON in #13).
add_custom_target(InstallRuncLib
        COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:runc>" "${PROJECT_SOURCE_DIR}/src/pyrunc/"
        COMMENT "Copying librunc to src/pyrunc"
        VERBATIM)
add_dependencies(InstallRuncLib runc)
