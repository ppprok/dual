cmake_minimum_required(VERSION 3.8)
project(dual)

set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ks thg.c)