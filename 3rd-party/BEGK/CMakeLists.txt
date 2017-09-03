cmake_minimum_required(VERSION 3.8)
project(dual)

set(CMAKE_CXX_STANDARD 14)
set(SOURCE_FILES dual.cpp
        box.h
        vector.c
        vector.h
        basic.c
        stack.h
        stack.c
        basic.h
        box.c)

add_executable(begk dual.cpp)