set(SOURCE_FILES
        Runc.h
        RuncFixed.h
        RuncFixed.cpp
        PUNC.cpp
        DualizationBacktrackAlgorithmBase.h
        Runc.cpp
        FileWriters.cpp
        FrequentInfrequentEnumerator.h
        Pruning.cpp
        BoolMatrixPreparation.h
        AOT1.cpp
        MostCoverCollector.cpp
        MostCoverCollector.h
        FileWriters.h
        WeightBoundsPruning.h
        WeightedCover.h
        Pruning.h
        AOT1.h
        DualizationCounters.h
        DualizationCounters.cpp
        BoolMatrixPreparation.cpp
        CoverCollector.cpp
        GeneticDualizer.h
        PUNC.h
        Weighters.h
        CoverCollector.h
        Weighters.cpp
        DualizationBacktrackAlgorithmBase.cpp
        Counters.h)


add_library(dual ${SOURCE_FILES})

target_link_libraries(dual INTERFACE common)

#add_executable(dualizer main.cpp)
#target_link_libraries(dualizer dual common pthread)

add_library(runc SHARED RuncExecutor.cpp)
target_link_libraries(runc dual common)
set_property(TARGET runc PROPERTY POSITION_INDEPENDENT_CODE TRUE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/librunc.so DESTINATION ${CMAKE_SOURCE_DIR}/src/pyrunc COMPONENT pyrunc)

add_custom_target(InstallRuncLib COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR} -DCOMPONENT=pyrunc -P ${CMAKE_BINARY_DIR}/cmake_install.cmake)

add_dependencies(InstallRuncLib runc)
