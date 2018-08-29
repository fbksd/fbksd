cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)
project(fbksd-core VERSION 0.0.1)

option(FBKSD_MANUAL_RENDERER "Enable manual renderer process." OFF)
option(FBKSD_MANUAL_CLIENT "Enable manual client process" OFF)
option(FBKSD_TESTS "Compile tests." OFF)

# turn on c++14
set(CMAKE_CXX_STANDARD 14)

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
add_subdirectory(src/benchmark)
add_subdirectory(src/compare)
add_subdirectory(src/exr2png)

# install results page
install(DIRECTORY ${CMAKE_SOURCE_DIR}/share/fbksd DESTINATION share USE_SOURCE_PERMISSIONS)

# configure and install fbksd.py script
configure_file(cmake/configure_script.cmake.in ${CMAKE_BINARY_DIR}/configure_script.cmake @ONLY)
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/src/fbksd.py
    COMMAND ${CMAKE_COMMAND} -P
        ${CMAKE_BINARY_DIR}/configure_script.cmake
    DEPENDS
        ${CMAKE_SOURCE_DIR}/src/fbksd.py.in
        ${CMAKE_SOURCE_DIR}/cmake/configure_script.cmake.in
        ${CMAKE_BINARY_DIR}/configure_script.cmake
    COMMENT "Configuring fbksd.py.in" VERBATIM
)
add_custom_target(configure_script ALL DEPENDS ${CMAKE_BINARY_DIR}/src/fbksd.py)
install(PROGRAMS ${CMAKE_BINARY_DIR}/src/fbksd.py DESTINATION bin RENAME fbksd)

# export targets
include("cmake/fbksdExport.cmake")
