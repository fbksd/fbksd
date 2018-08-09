
include(CMakeFindDependencyMacro)
find_dependency(Qt5Core)
find_dependency(Qt5Network)
find_dependency(Qt5Xml)
include("${CMAKE_CURRENT_LIST_DIR}/fbksdTargets.cmake")
