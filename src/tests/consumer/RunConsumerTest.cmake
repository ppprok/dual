# Script behind the CTest test dual_consumer: installs the current build into a fresh prefix, then
# configures, builds and runs the find_package(dual) project in this directory against it.
#
#   cmake -DDUAL_BUILD_DIR=<build> -DDUAL_CONFIG=<config> -DWORK_DIR=<dir> -DGENERATOR=<generator>
#         -DCXX_COMPILER=<c++> [-DMAKE_PROGRAM=<ninja/make>] [-DLINK_FLAGS=<flags>] -P RunConsumerTest.cmake
#
# LINK_FLAGS carries the sanitizer runtime for a DUAL_ENABLE_SANITIZERS build: the installed static
# libraries are instrumented, but the exported targets do not propagate -fsanitize.
foreach(var DUAL_BUILD_DIR DUAL_CONFIG WORK_DIR GENERATOR CXX_COMPILER)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "RunConsumerTest.cmake: ${var} is not set")
    endif()
endforeach()

set(prefix "${WORK_DIR}/prefix")
set(build "${WORK_DIR}/build")
file(REMOVE_RECURSE "${WORK_DIR}")

function(run)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        string(REPLACE ";" " " cmd "${ARGN}")
        message(FATAL_ERROR "Failed (${result}): ${cmd}")
    endif()
endfunction()

run("${CMAKE_COMMAND}" --install "${DUAL_BUILD_DIR}" --prefix "${prefix}" --config "${DUAL_CONFIG}")
run("${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}" -B "${build}" -G "${GENERATOR}"
        "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}"
        "-DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}"
        "-DCMAKE_BUILD_TYPE=${DUAL_CONFIG}"
        "-DCMAKE_PREFIX_PATH=${prefix}"
        "-DCMAKE_EXE_LINKER_FLAGS=${LINK_FLAGS}")
run("${CMAKE_COMMAND}" --build "${build}" --config "${DUAL_CONFIG}")

set(consumer "")
foreach(dir "${build}" "${build}/${DUAL_CONFIG}")  # single- and multi-config generators
    foreach(name dual_consumer dual_consumer.exe)
        if(NOT consumer AND EXISTS "${dir}/${name}")
            set(consumer "${dir}/${name}")
        endif()
    endforeach()
endforeach()
if(NOT consumer)
    message(FATAL_ERROR "dual_consumer was not built in ${build}")
endif()
run("${consumer}")
