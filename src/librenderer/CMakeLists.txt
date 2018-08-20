
# Qt
set(CMAKE_AUTOMOC ON)
find_package(Qt5Core)

set(HEADERS_PREFIX ${PROJECT_SOURCE_DIR}/include/fbksd/renderer)

# header files
set(HEADERS ${HEADERS_PREFIX}/RenderingServer.h
            ${HEADERS_PREFIX}/samples.h)

# source files
set(SRCS RenderingServer.cpp
         samples.cpp )

add_library(renderer ${SRCS} ${HEADERS})
add_library(fbksd::renderer ALIAS renderer)
target_link_libraries(renderer PUBLIC core)
set_target_properties(renderer PROPERTIES OUTPUT_NAME "fbksd-renderer")
install(TARGETS renderer
    EXPORT fbksdTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY ${HEADERS_PREFIX}
    DESTINATION include/fbksd
)
