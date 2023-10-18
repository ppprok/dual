cmake_minimum_required(VERSION 3.8)
project(dual)

set(CMAKE_CXX_STANDARD 14)

#set(Boost_USE_STATIC_LIBS ON) # only find static libs
#set(Boost_USE_MULTITHREADED ON)
#set(Boost_USE_STATIC_RUNTIME OFF)
#
#find_package(Boost 1.67.0 REQUIRED COMPONENTS filesystem log log_setup program_options)
#message(STATUS ${Boost_LIBRARIES})

#add_subdirectory(3rd-party/BEGK)
#add_subdirectory(3rd-party/BMR)
#add_subdirectory(3rd-party/DL)
#add_subdirectory(3rd-party/KS)
#add_subdirectory(3rd-party/shd31)
#add_subdirectory(3rd-party/mtminer-dist)
#add_subdirectory(3rd-party/boost)
add_subdirectory(src)

