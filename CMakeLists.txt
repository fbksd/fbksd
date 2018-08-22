cmake_minimum_required(VERSION 2.8.3 FATAL_ERROR)
project(fbksd-core)

option(FBKSD_MANUAL_RENDERER "Enable manual renderer process." OFF)
option(FBKSD_MANUAL_CLIENT "Enable manual client process" OFF)

# turn on c++14
set(CMAKE_CXX_STANDARD 14)

# rpclib lib
add_subdirectory(ext/rpclib)
add_library(rpclib::rpc ALIAS rpc)

add_subdirectory(src/libcore)
add_subdirectory(src/libclient)
add_subdirectory(src/librenderer)
add_subdirectory(src/benchmark)
add_subdirectory(src/compare)
add_subdirectory(src/exr2png)

include("cmake/fbksdExport.cmake")
