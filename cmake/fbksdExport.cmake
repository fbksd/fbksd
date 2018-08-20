
include(CMakePackageConfigHelpers)

install(EXPORT fbksdTargets
    FILE fbksdTargets.cmake
    NAMESPACE fbksd::
    DESTINATION lib/cmake/fbksd
)
write_basic_package_version_file("fbksdConfigVersion.cmake"
    VERSION 1.0
    COMPATIBILITY SameMajorVersion
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/fbksdConfigVersion.cmake"
        "${CMAKE_CURRENT_LIST_DIR}/fbksdConfig.cmake"
    DESTINATION
        lib/cmake/fbksd
)
