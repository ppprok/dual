set(DUAL_DUAL_HEADERS
        AOT1.h
        BoolMatrixPreparation.h
        Counters.h
        CoverCollector.h
        DualizationBacktrackAlgorithmBase.h
        DualizationCounters.h
        FileWriters.h
        MostCoverCollector.h
        Pruning.h
        PUNC.h
        Runc.h
        RuncFixed.h
        WeightBoundsPruning.h
        WeightedCover.h
        Weighters.h)
list(TRANSFORM DUAL_DUAL_HEADERS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set(DUAL_DUAL_HEADERS ${DUAL_DUAL_HEADERS} PARENT_SCOPE)
set(DUAL_RUNC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/RuncExecutor.h" PARENT_SCOPE)

# Static library with the dualization algorithms.
add_library(dual
        AOT1.cpp
        BoolMatrixPreparation.cpp
        CoverCollector.cpp
        DualizationBacktrackAlgorithmBase.cpp
        DualizationCounters.cpp
        FileWriters.cpp
        MostCoverCollector.cpp
        Pruning.cpp
        PUNC.cpp
        Runc.cpp
        RuncFixed.cpp
        Weighters.cpp
        ${DUAL_DUAL_HEADERS})
add_library(dual::dual ALIAS dual)

target_include_directories(dual PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/dual>)
target_link_libraries(dual PUBLIC dual::common)
set_target_properties(dual PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME dual)
dual_target_options(dual)

# Shared library with the C ABI used by src/pyrunc/runc.py via ctypes.
find_package(Threads REQUIRED)
add_library(runc SHARED RuncExecutor.cpp RuncExecutor.h)
add_library(dual::runc ALIAS runc)
target_include_directories(runc PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/dual>)
target_link_libraries(runc PRIVATE dual::dual Threads::Threads)
set_target_properties(runc PROPERTIES EXPORT_NAME runc)
dual_target_options(runc)

# Copies librunc.so next to src/pyrunc/runc.py (to be replaced by DUAL_BUILD_PYTHON in #13).
add_custom_target(InstallRuncLib
        COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:runc>" "${PROJECT_SOURCE_DIR}/src/pyrunc/"
        COMMENT "Copying librunc to src/pyrunc"
        VERBATIM)
add_dependencies(InstallRuncLib runc)
