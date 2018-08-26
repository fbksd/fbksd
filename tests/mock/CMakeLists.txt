
find_package(Qt5Core REQUIRED)

add_executable(mockrenderer mockrenderer.cpp)
target_link_libraries(mockrenderer PRIVATE fbksd::renderer Qt5::Core)

add_executable(mockclient mockclient.cpp)
target_link_libraries(mockclient PRIVATE fbksd::client)
