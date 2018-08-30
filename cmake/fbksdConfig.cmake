
include(CMakeFindDependencyMacro)
find_dependency(Qt5Core)
find_dependency(Threads)
find_dependency(rpclib)
find_dependency(Boost COMPONENTS program_options system)
include("${CMAKE_CURRENT_LIST_DIR}/fbksdTargets.cmake")
