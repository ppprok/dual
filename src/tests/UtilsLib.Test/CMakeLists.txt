add_executable(TestUtils
        main.cpp
        BitChunkTests.cpp
        BitsTests.cpp
        CSVTests.cpp
        GraphTests.cpp
        IOTests.cpp
        RegistratorTests.cpp)

add_dependencies(TestUtils doctest)
target_link_libraries(TestUtils PRIVATE common)
target_include_directories(TestUtils PRIVATE ${DOCTEST_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)

add_dependencies(TestUtils InstallTestUtilsData)