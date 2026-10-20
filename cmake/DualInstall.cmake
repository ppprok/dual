# Install rules and the CMake package (find_package(dual) -> dual::common, dual::dual, dual::runc).
include(CMakePackageConfigHelpers)

set(DUAL_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/dual" CACHE STRING "Install path for dualConfig.cmake")

install(TARGETS common dual runc
        EXPORT dualTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dual)

# Headers are flat for now (they include each other as "bit_matrix.h"); layout moves to include/ in #12.
install(FILES ${DUAL_COMMON_HEADERS} ${DUAL_DUAL_HEADERS} ${DUAL_RUNC_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dual)

install(EXPORT dualTargets
        NAMESPACE dual::
        DESTINATION ${DUAL_INSTALL_CMAKEDIR})

configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/dualConfig.cmake.in"
        "${PROJECT_BINARY_DIR}/dualConfig.cmake"
        INSTALL_DESTINATION ${DUAL_INSTALL_CMAKEDIR})
write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/dualConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMinorVersion)

install(FILES "${PROJECT_BINARY_DIR}/dualConfig.cmake" "${PROJECT_BINARY_DIR}/dualConfigVersion.cmake"
        DESTINATION ${DUAL_INSTALL_CMAKEDIR})
