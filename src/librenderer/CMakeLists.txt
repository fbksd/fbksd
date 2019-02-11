
set(HEADERS_PREFIX ${PROJECT_SOURCE_DIR}/include/fbksd/renderer)

# header files
set(HEADERS ${HEADERS_PREFIX}/RenderingServer.h
            ${HEADERS_PREFIX}/samples.h
            ${HEADERS_PREFIX}/SamplesPipe.h
            TilePool.h)

# source files
set(SRCS RenderingServer.cpp
         SamplesPipe.cpp
         samples.cpp
         TilePool.cpp )

add_library(renderer SHARED ${SRCS} ${HEADERS})
add_library(fbksd::renderer ALIAS renderer)
target_link_libraries(renderer PUBLIC core)
set_target_properties(renderer PROPERTIES
    OUTPUT_NAME "fbksd-renderer"
    VERSION ${PROJECT_VERSION_MAJOR}
)
target_include_directories(renderer PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
install(TARGETS renderer
    EXPORT fbksdTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY ${HEADERS_PREFIX}
    DESTINATION include/fbksd
)
