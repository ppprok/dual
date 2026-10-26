# Common compile settings for the project's own targets (not for third-party code).
#
#   dual_target_options(<target>)
#
# Sets C++17, warnings (-Wall -Wextra, /W4 on MSVC), optional -Werror and optional sanitizers
# according to DUAL_WARNINGS_AS_ERRORS, DUAL_ENABLE_SANITIZERS and DUAL_ENABLE_COVERAGE.
#
# DUAL_WARNINGS_AS_ERRORS applies to GCC and Clang (incl. Apple Clang), which CI keeps warning-free.
# MSVC /W4 still reports the int-sized index/size design (C4244, C4267, C4245), fopen (C4996) and
# shadowing (C4456), so /WX is not added there; the warnings stay visible in the build log.
function(dual_target_options target)
    target_compile_features(${target} PUBLIC cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra $<$<BOOL:${DUAL_WARNINGS_AS_ERRORS}>:-Werror>)
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
