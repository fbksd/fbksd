
find_package(Threads)

set(HEADERS_PREFIX ${PROJECT_SOURCE_DIR}/include/fbksd/core)

# header files
set(HEADERS ${HEADERS_PREFIX}/Point.h
            ${HEADERS_PREFIX}/Definitions.h
            ${HEADERS_PREFIX}/SampleLayout.h
            ${HEADERS_PREFIX}/SceneInfo.h
            ${HEADERS_PREFIX}/SharedMemory.h)

# source files
set(SRCS SampleLayout.cpp
         SceneInfo.cpp
         SharedMemory.cpp)

add_library(core ${SRCS} ${HEADERS})
add_library(fbksd::core ALIAS core)
target_link_libraries(core PUBLIC -lrt rpclib::rpc Threads::Threads)
set_target_properties(core PROPERTIES OUTPUT_NAME "fbksd-core")
target_include_directories(core PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS core
    EXPORT fbksdTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY ${HEADERS_PREFIX}
    DESTINATION include/fbksd
)
