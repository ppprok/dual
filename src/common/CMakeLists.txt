set(DUAL_COMMON_HEADERS
        bit_chunk.h
        bit_mask.h
        bit_matrix.h
        bit_vector.h
        bits.h
        blob.h
        chunk.h
        containers.h
        graph_algorithms.h
        IntegersFileReader.h
        IntegersFileWriter.h
        io_utils.h
        IteratorRange.h
        marked.h
        math_utils.h
        Randomizer.h
        range.h
        raw_vector.h
        string_utils.h)
list(TRANSFORM DUAL_COMMON_HEADERS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set(DUAL_COMMON_HEADERS ${DUAL_COMMON_HEADERS} PARENT_SCOPE)

add_library(common
        bits.cpp
        graph_algorithms.cpp
        IntegersFileReader.cpp
        IntegersFileWriter.cpp
        io_utils.cpp
        ${DUAL_COMMON_HEADERS})
add_library(dual::common ALIAS common)

target_include_directories(common PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/dual>)
set_target_properties(common PROPERTIES
        POSITION_INDEPENDENT_CODE ON  # linked into the shared librunc.so
        EXPORT_NAME common
        OUTPUT_NAME dual_common)      # avoid a generic libcommon.a in lib/
dual_target_options(common)
