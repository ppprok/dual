# Common compile settings for the project's own targets (not for third-party code).
#
#   dual_target_options(<target>)
#
# Sets C++17, warnings (-Wall -Wextra, /W4 on MSVC), optional -Werror and optional sanitizers
# according to DUAL_WARNINGS_AS_ERRORS and DUAL_ENABLE_SANITIZERS.
function(dual_target_options target)
    target_compile_features(${target} PUBLIC cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 $<$<BOOL:${DUAL_WARNINGS_AS_ERRORS}>:/WX>)
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
endfunction()
