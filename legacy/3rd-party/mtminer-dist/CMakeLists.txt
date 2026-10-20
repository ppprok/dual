cmake_minimum_required(VERSION 3.8)
project(dual)

set(CMAKE_CXX_STANDARD 14)
set(SOURCE_FILES support.h
        ptreelibre.h
        ptreelibre.cpp
        dataset.h
        support.cpp
        dataset.cpp
        algo.cpp
        algo.h
        main.cpp
        constantes.h)

add_executable(mtminer-dist ${SOURCE_FILES})