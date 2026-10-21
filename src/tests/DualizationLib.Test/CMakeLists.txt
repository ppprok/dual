# RuncExecutor.cpp is compiled in, so the C ABI is tested without loading librunc.so.
dual_add_test(TestDual
        SOURCES
        main.cpp
        RUNCTests.cpp
        RuncApiTests.cpp
        RuncDifferentialTests.cpp
        RuncTestUtils.h
        "${PROJECT_SOURCE_DIR}/src/dual/RuncExecutor.cpp"
        LIBS dual::dual)
