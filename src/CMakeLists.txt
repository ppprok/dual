add_subdirectory(common)
add_subdirectory(dual)

# Header lists for install rules (cmake/DualInstall.cmake).
set(DUAL_COMMON_HEADERS ${DUAL_COMMON_HEADERS} PARENT_SCOPE)
set(DUAL_DUAL_HEADERS ${DUAL_DUAL_HEADERS} PARENT_SCOPE)
set(DUAL_RUNC_HEADERS ${DUAL_RUNC_HEADERS} PARENT_SCOPE)
