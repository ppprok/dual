cmake_minimum_required(VERSION 3.8)
project(dual)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(3rd-party/BEGK)
add_subdirectory(3rd-party/BMR)
add_subdirectory(3rd-party/DL)
add_subdirectory(3rd-party/KS)
add_subdirectory(3rd-party/shd31)
add_subdirectory(3rd-party/mtminer-dist)
add_subdirectory(3rd-party/boost)
add_subdirectory(src)

