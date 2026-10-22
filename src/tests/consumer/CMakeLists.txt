# Stand-alone consumer of an installed dual package, built by RunConsumerTest.cmake (CTest test
# dual_consumer). It uses symbols that once failed to link for consumers (#34).
cmake_minimum_required(VERSION 3.16...3.31)
project(dual_consumer LANGUAGES CXX)

find_package(dual 0.1 REQUIRED CONFIG)

add_executable(dual_consumer main.cpp)
target_link_libraries(dual_consumer PRIVATE dual::dual)
