# dual_bench: times one algorithm on one matrix (see benchmarks/README.md). Built only with
# DUAL_BUILD_BENCHMARKS=ON (the CMake presets turn it on, so CI builds it); not installed.
add_executable(dual_bench dual_bench.cpp)
target_link_libraries(dual_bench PRIVATE dual::dual)
dual_target_options(dual_bench)
