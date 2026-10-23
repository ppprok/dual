# runc_executor.cpp is compiled in, so the C ABI is tested without loading librunc.so.
dual_add_test(TestDual
        SOURCES
        main.cpp
        RUNCTests.cpp
        RuncApiTests.cpp
        RuncDifferentialTests.cpp
        RuncTestUtils.h
        DualTestUtils.h
        AlgorithmMatrixTests.cpp
        CallbackTests.cpp
        "${PROJECT_SOURCE_DIR}/src/dual/runc_executor.cpp"
        AOT1PUNCTests.cpp
        PruningTests.cpp
        CoverCollectorTests.cpp
        FKTests.cpp
        LIBS dual::dual)

# runc_fixed.cpp compiled with -O2 into its own client, independently of the build type: the link
# fails unless RuncFixed members and RuncFixedDualize are explicitly instantiated (#23, #34).
dual_add_test(TestRuncFixedO2
        SOURCES
        main.cpp
        RuncFixedO2Tests.cpp
        "${PROJECT_SOURCE_DIR}/src/dual/runc_fixed.cpp"
        LIBS dual::common)
# MSVC rejects /O2 together with the /RTC1 of Debug builds; /Ob2 still inlines aggressively.
target_compile_options(TestRuncFixedO2 PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/Ob2,-O2>)
