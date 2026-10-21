# API documentation (DUAL_BUILD_DOCS): target `dual_docs` runs Doxygen with docs/Doxyfile and writes
# HTML to <build dir>/docs/html. Without Doxygen the option only warns, so configure still succeeds.
find_package(Doxygen)

if(NOT DOXYGEN_FOUND)
    message(WARNING "DUAL_BUILD_DOCS is ON, but Doxygen was not found: the dual_docs target is not available.")
    return()
endif()

# docs/Doxyfile uses paths relative to the source root; only the output directory is overridden.
set(DUAL_DOCS_OUTPUT_DIR "${PROJECT_BINARY_DIR}/docs")
file(WRITE "${PROJECT_BINARY_DIR}/Doxyfile.dual"
        "@INCLUDE = \"${PROJECT_SOURCE_DIR}/docs/Doxyfile\"\n"
        "OUTPUT_DIRECTORY = \"${DUAL_DOCS_OUTPUT_DIR}\"\n")

add_custom_target(dual_docs
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DUAL_DOCS_OUTPUT_DIR}"
        COMMAND Doxygen::doxygen "${PROJECT_BINARY_DIR}/Doxyfile.dual"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Generating API documentation in ${DUAL_DOCS_OUTPUT_DIR}/html"
        VERBATIM)
