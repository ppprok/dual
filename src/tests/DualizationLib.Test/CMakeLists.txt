# RuncExecutor.cpp is compiled in, so the C ABI is tested without loading librunc.so.
dual_add_test(TestDual
        SOURCES
        main.cpp
        RUNCTests.cpp
        RuncApiTests.cpp
        RuncDifferentialTests.cpp
        RuncTestUtils.h
        "${PROJECT_SOURCE_DIR}/src/dual/RuncExecutor.cpp"
        AOT1PUNCTests.cpp
        CoverCollectorTests.cpp
        LIBS dual::dual)

# RuncFixed.cpp compiled with -O2 into its own client, independently of the build type: the link
# fails unless RuncFixed members are explicitly instantiated (#23).
dual_add_test(TestRuncFixedO2
        SOURCES
        main.cpp
        RuncFixedO2Tests.cpp
        "${PROJECT_SOURCE_DIR}/src/dual/RuncFixed.cpp"
        LIBS dual::common)
target_include_directories(TestRuncFixedO2 PRIVATE "${PROJECT_SOURCE_DIR}/src/dual")
target_compile_options(TestRuncFixedO2 PRIVATE -O2)
