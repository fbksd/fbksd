
# Qt
find_package(Qt5Core)

# OpenCV
find_package(OpenCV REQUIRED)

# source files
set(SRCS exr2png.cpp)

add_executable(fbksd-exr2png ${SRCS})
target_link_libraries(fbksd-exr2png PRIVATE ${OpenCV_LIBS} Qt5::Core)
install(TARGETS fbksd-exr2png
    RUNTIME DESTINATION bin
)
