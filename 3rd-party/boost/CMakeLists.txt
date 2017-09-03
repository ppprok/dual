cmake_minimum_required(VERSION 3.5)
project(boost)

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

set(BOOST_MAKE_THREADS -j3)

set(BOOST_CXX_FLAGS -std=c++14)

if(UNIX)  # Linux:)

    set(BOOST_TARGET_OS target-os=linux)
    set(BOOST_ADDRESS_MODEL address-model=32_64)

    set(BOOST_TOOLSET toolset=gcc-cmake)
    configure_file(scripts/build.in.sh ${CMAKE_CURRENT_BINARY_DIR}/scripts/build.sh)
    configure_file(scripts/user-config.jam.linux
            ${CMAKE_CURRENT_BINARY_DIR}/share/boost-build/src/user-config.jam)

else()
    message(SEND_ERROR "Unknown target OS")
endif()


set(BOOST_ARCHITECTURE architecture=${CMAKE_SYSTEM_PROCESSOR})
if(CMAKE_SYSTEM_PROCESSOR STREQUAL i386)
    set(BOOST_ARCHITECTURE architecture=x86)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL arm)
    set(BOOST_LITTLE_ENDIAN define=_LITTLE_ENDIAN)
endif()

include(ExternalProject)
ExternalProject_Add(boost_external
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/vendor
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/scripts/build.sh <SOURCE_DIR> <BINARY_DIR>
        INSTALL_COMMAND "")

add_library(boost INTERFACE)
add_dependencies(boost boost_external)
target_include_directories(boost INTERFACE vendor)
target_link_libraries(boost INTERFACE
        #${CMAKE_CURRENT_BINARY_DIR}/lib/libboost_atomic.a
        #${CMAKE_CURRENT_BINARY_DIR}/lib/libboost_chrono.a
        #${CMAKE_CURRENT_BINARY_DIR}/lib/libboost_date_time.a
        ${CMAKE_CURRENT_BINARY_DIR}/lib/libboost_log_setup.a
        ${CMAKE_CURRENT_BINARY_DIR}/lib/libboost_log.a
#        ${CMAKE_CURRENT_BINARY_DIR}/lib/libboost_filesystem.a
        #${CMAKE_CURRENT_BINARY_DIR}/lib/libboost_random.a
#        ${CMAKE_CURRENT_BINARY_DIR}/lib/libboost_regex.a
#        ${CMAKE_CURRENT_BINARY_DIR}/lib/libboost_system.a
#        ${CMAKE_CURRENT_BINARY_DIR}/lib/libboost_thread.a
        #${CMAKE_CURRENT_BINARY_DIR}/lib/libboost_timer.a
#        openssl
        )

