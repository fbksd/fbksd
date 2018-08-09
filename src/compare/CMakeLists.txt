
# Qt
find_package(Qt5Core)

# OpenCV
find_package(OpenCV REQUIRED)

# source files
set(SRCS compare.cpp)

add_executable(fbksd-compare ${SRCS})
target_link_libraries(fbksd-compare PRIVATE ${OpenCV_LIBS} Qt5::Core)
install(TARGETS fbksd-compare
    RUNTIME DESTINATION bin
)
