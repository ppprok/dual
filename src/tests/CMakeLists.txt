include(ExternalProject)
find_package(Git REQUIRED)

ExternalProject_Add(
        doctest
        PREFIX ${CMAKE_BINARY_DIR}/doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        TIMEOUT 10
        UPDATE_COMMAND ${GIT_EXECUTABLE} pull
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
)

# Expose required variable (DOCTEST_INCLUDE_DIR) to parent scope
ExternalProject_Get_Property(doctest source_dir)
set(DOCTEST_INCLUDE_DIR ${source_dir}/doctest CACHE INTERNAL "Path to include folder for doctest")

install(FILES data/winequality-red.csv
        data/A1.csv
        data/A2.csv
        data/A3.csv
        data/A4.csv
        data/A5.csv
        data/A6.csv
        data/A7.csv
        data/achain1.info
        data/antichains-30.json
        data/antichains-40x30-bin-10-2.csv
        data/B1.csv
        data/chaines-3.info
        data/dual-posets-task1.info
        data/dual-posets-task2.info
        data/dual-posets-task3.info
        data/dual-posets-task4.info
        data/dual-posets-task5.info
        data/fif-task1.info
        data/graphposet1.json
        data/intervals.csv
        data/intervals2.csv
        data/numberchains-10.json
        data/numberchains-20x10-norm-0-1.csv
        data/numberchains-30.json
        data/numberchains-40x10-norm-0-1.csv
        data/numberchains-40x30-norm-0-1.csv
        data/poset1.info
        data/pricegraph-5.json
        data/pricegraph-5x5-uniform-10.csv
        data/pricegraph-10.json
        data/pricegraph-10-ideal-10.json
        data/pricegraph-10x10-filter-851.csv
        data/pricegraph-10x10-uniform-10.csv
        data/pricegraph-30.json
        data/pricegraph-40x30-uniform-10.csv
        data/6_6_6.hg
        data/integers.txt
        data/write_bm_test.hg
        data/10_25_480.hg
        data/6_6_4.hg
        data/10_50_7187.hg
        data/13_7_10.hg
        data/15_10_14.hg
        data/15_20_275.hg
        data/15_35_12226.hg
        data/iris.txt
        DESTINATION data COMPONENT TestUtilsData)

add_custom_target(InstallTestUtilsData COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR} -DCOMPONENT=TestUtilsData -P ${CMAKE_BINARY_DIR}/cmake_install.cmake)


add_subdirectory(LogicalAnalysisLib.Test)
add_subdirectory(UtilsLib.Test)
add_subdirectory(DualizationLib.Test)

set_target_properties( TestDual TestUtils TestLogical
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
