
add_executable(mockrenderer mockrenderer.cpp)
target_link_libraries(mockrenderer PRIVATE fbksd::renderer)

add_executable(mockclient mockclient.cpp)
target_link_libraries(mockclient PRIVATE fbksd::client)
