cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)
project(fbksd-core VERSION 2.3.0)

option(FBKSD_TESTS "Compile tests." OFF)
option(FBKSD_PYTHON "Compile python bindings." ON)
if(FBKSD_PYTHON)
    set(FBKSD_PYTHON_PATH "lib/python3/dist-packages" CACHE PATH
        "Path where FBKSD python package will be installed. The given path is appended to CMAKE_INSTALL_PREFIX to form the final location.")
    message("Python bindins enabled. Will be installed at: " "${CMAKE_INSTALL_PREFIX}/${FBKSD_PYTHON_PATH}")
endif()


# turn on c++14
set(CMAKE_CXX_STANDARD 14)

set(BUILD_SHARED_LIBS ON)

if(FBKSD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# rpclib lib
add_subdirectory(ext/rpclib)
add_library(rpclib::rpc ALIAS rpc)

add_subdirectory(src/libcore)
add_subdirectory(src/libbenchmark)
add_subdirectory(src/libclient)
add_subdirectory(src/librenderer)
add_subdirectory(src/libiqa)
if(FBKSD_PYTHON)
    add_subdirectory(src/libpython)
endif()
add_subdirectory(src/benchmark)
add_subdirectory(src/iqa)
add_subdirectory(src/exr2png)

# install data folder
install(DIRECTORY ${CMAKE_SOURCE_DIR}/share/fbksd DESTINATION share USE_SOURCE_PERMISSIONS)
# install bin/fbksd symlink
install(PROGRAMS ${CMAKE_SOURCE_DIR}/util/fbksd DESTINATION bin)
# configure/install export_path.sh script
configure_file(util/export_path.sh.in ${CMAKE_BINARY_DIR}/export_path.sh)
install(PROGRAMS ${CMAKE_BINARY_DIR}/export_path.sh DESTINATION share/fbksd)
# export targets
include("cmake/fbksdExport.cmake")

