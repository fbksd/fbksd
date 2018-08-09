cmake_minimum_required(VERSION 2.8.3 FATAL_ERROR)
project(fbksd-core)

#add_definitions(-DMANUAL_RENDERER)
#add_definitions(-DMANUAL_ASR)

# turn on c++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(src/libcore)
add_subdirectory(src/libclient)
add_subdirectory(src/librenderer)
add_subdirectory(src/benchmark)
add_subdirectory(src/compare)
add_subdirectory(src/exr2png)

install(EXPORT fbksdTargets
    FILE fbksdTargets.cmake
    NAMESPACE fbksd::
    DESTINATION lib/cmake/fbksd
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("fbksdConfigVersion.cmake"
    VERSION 1.0
    COMPATIBILITY SameMajorVersion
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/fbksdConfigVersion.cmake" "cmake/fbksdConfig.cmake"
    DESTINATION lib/cmake/fbksd
)
