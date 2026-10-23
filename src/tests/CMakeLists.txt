include(FetchContent)

FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.5.3
        GIT_SHALLOW TRUE)
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(doctest)
include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")

# Tests open files by relative path ("data/6_6_4.hg", "temp/..."), so they run from this
# binary directory, where dual_test_data mirrors src/tests/data/.
set(DUAL_TEST_WORKING_DIR "${CMAKE_CURRENT_BINARY_DIR}")
add_custom_target(dual_test_data
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" "${DUAL_TEST_WORKING_DIR}/data"
        COMMENT "Copying test data"
        VERBATIM)

# dual_add_test(<name> SOURCES <src>... LIBS <lib>...)
# Adds a doctest executable that runs from DUAL_TEST_WORKING_DIR and registers its cases with CTest.
function(dual_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
    add_executable(${name} ${ARG_SOURCES})
    # Public headers come as <dual/...> from the library targets; internal ones as "common/...", relative to src/.
    target_include_directories(${name} PRIVATE "${PROJECT_SOURCE_DIR}/src")
    target_link_libraries(${name} PRIVATE ${ARG_LIBS} doctest::doctest)
    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${DUAL_TEST_WORKING_DIR}")
    add_dependencies(${name} dual_test_data)
    dual_target_options(${name})
    doctest_discover_tests(${name}
            TEST_PREFIX "${name}."
            WORKING_DIRECTORY "${DUAL_TEST_WORKING_DIR}")
endfunction()

add_subdirectory(UtilsLib.Test)
add_subdirectory(DualizationLib.Test)

# dual_consumer: install this build into a scratch prefix and build consumer/ against it with
# find_package(dual), so symbols that only fail to link outside the build tree are caught (#34).
# Runs after the build, needs no network (the consumer has no dependencies besides dual).
if(DUAL_INSTALL)
    set(_consumer_link_flags "")
    if(DUAL_ENABLE_SANITIZERS AND NOT MSVC)
        set(_consumer_link_flags "-fsanitize=address,undefined")
    endif()
    if(DUAL_ENABLE_COVERAGE)
        string(STRIP "${_consumer_link_flags} --coverage" _consumer_link_flags)
    endif()
    add_test(NAME dual_consumer
            COMMAND "${CMAKE_COMMAND}"
            "-DDUAL_BUILD_DIR=${PROJECT_BINARY_DIR}"
            "-DDUAL_CONFIG=$<CONFIG>"
            "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/consumer"
            "-DGENERATOR=${CMAKE_GENERATOR}"
            "-DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
            "-DCXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DLINK_FLAGS=${_consumer_link_flags}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/consumer/RunConsumerTest.cmake")
endif()
