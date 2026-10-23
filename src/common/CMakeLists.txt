# Public headers of dual::common (installed, included as <dual/...>).
set(DUAL_COMMON_PUBLIC_HEADERS
        bit_chunk.h
        bit_mask.h
        bit_matrix.h
        bit_vector.h
        bits.h
        blob.h
        chunk.h
        containers.h
        integers_file_reader.h
        integers_file_writer.h
        io_utils.h
        marked.h
        range.h
        raw_vector.h)
list(TRANSFORM DUAL_COMMON_PUBLIC_HEADERS PREPEND "${PROJECT_SOURCE_DIR}/include/dual/")

# Internal headers: not installed; tests include them as "common/...". csv_loader.cpp is not compiled
# (its header-only parts are used by TestUtils).
set(DUAL_COMMON_INTERNAL_HEADERS
        csv_loader.h
        graph_algorithms.h
        iterator_range.h
        math_utils.h
        randomizer.h
        registrator.h
        string_utils.h)

add_library(common
        bits.cpp
        graph_algorithms.cpp
        integers_file_reader.cpp
        integers_file_writer.cpp
        io_utils.cpp
        ${DUAL_COMMON_PUBLIC_HEADERS}
        ${DUAL_COMMON_INTERNAL_HEADERS})
add_library(dual::common ALIAS common)

target_include_directories(common PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_target_properties(common PROPERTIES
        POSITION_INDEPENDENT_CODE ON  # linked into the shared librunc.so
        EXPORT_NAME common
        OUTPUT_NAME dual_common)      # avoid a generic libcommon.a in lib/
dual_target_options(common)
