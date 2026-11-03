# Common compile settings for the project's own targets (not for third-party code).
#
#   dual_target_options(<target>)
#
# Sets C++17, warnings (-Wall -Wextra, /W4 on MSVC), optional -Werror, optional sanitizers and optional
# CPU-specific code generation according to DUAL_WARNINGS_AS_ERRORS, DUAL_ENABLE_SANITIZERS,
# DUAL_ENABLE_COVERAGE and DUAL_NATIVE_ARCH.
#
# DUAL_WARNINGS_AS_ERRORS applies to GCC and Clang (incl. Apple Clang), which CI keeps warning-free.
# MSVC /W4 still reports the int-sized index/size design (C4244, C4267, C4245), fopen (C4996) and
# shadowing (C4456), so /WX is not added there; the warnings stay visible in the build log.
# DUAL_NATIVE_ARCH: DUAL_ARCH_FLAGS if set, otherwise -march=native (-mcpu=native where the compiler
# has no -march=native, e.g. older Apple Clang on arm64) or /arch:AVX2 on MSVC. The flags are PRIVATE:
# they are not exported to consumers of the installed package.
set(DUAL_ARCH_COMPILE_OPTIONS "")
if(DUAL_NATIVE_ARCH)
    if(DUAL_ARCH_FLAGS)
        separate_arguments(DUAL_ARCH_COMPILE_OPTIONS NATIVE_COMMAND "${DUAL_ARCH_FLAGS}")
    elseif(MSVC)
        set(DUAL_ARCH_COMPILE_OPTIONS /arch:AVX2)
    else()
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag(-march=native DUAL_HAS_MARCH_NATIVE)
        if(DUAL_HAS_MARCH_NATIVE)
            set(DUAL_ARCH_COMPILE_OPTIONS -march=native)
        else()
            check_cxx_compiler_flag(-mcpu=native DUAL_HAS_MCPU_NATIVE)
            if(DUAL_HAS_MCPU_NATIVE)
                set(DUAL_ARCH_COMPILE_OPTIONS -mcpu=native)
            else()
                message(WARNING "DUAL_NATIVE_ARCH: the compiler accepts neither -march=native nor -mcpu=native; "
                        "set DUAL_ARCH_FLAGS")
            endif()
        endif()
    endif()
    message(STATUS "DUAL_NATIVE_ARCH: own targets are compiled with '${DUAL_ARCH_COMPILE_OPTIONS}'")
endif()

function(dual_target_options target)
    target_compile_features(${target} PUBLIC cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra $<$<BOOL:${DUAL_WARNINGS_AS_ERRORS}>:-Werror>)
    endif()

    if(DUAL_ARCH_COMPILE_OPTIONS)
        target_compile_options(${target} PRIVATE ${DUAL_ARCH_COMPILE_OPTIONS})
    endif()

    if(DUAL_ENABLE_SANITIZERS)
        if(MSVC)
            target_compile_options(${target} PRIVATE /fsanitize=address)
        else()
            set(_san -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
            target_compile_options(${target} PRIVATE ${_san})
            # Link options propagate to consumers of static libs so the final executable gets the runtime.
            target_link_options(${target} PUBLIC $<BUILD_INTERFACE:-fsanitize=address,undefined>)
        endif()
    endif()

    # gcov instrumentation (gcc/clang only): every own object writes .gcda files next to its .o when a
    # test runs; gcovr reads them (see the coverage preset and src/tests/README.md).
    if(DUAL_ENABLE_COVERAGE)
        if(MSVC)
            message(FATAL_ERROR "DUAL_ENABLE_COVERAGE needs gcc or clang (--coverage)")
        endif()
        target_compile_options(${target} PRIVATE --coverage -O0)
        target_link_options(${target} PUBLIC $<BUILD_INTERFACE:--coverage>)
    endif()
endfunction()
