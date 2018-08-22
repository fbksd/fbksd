

set(HEADERS_PREFIX ${PROJECT_SOURCE_DIR}/include/fbksd/client)

# header files
set(HEADERS ${HEADERS_PREFIX}/BenchmarkClient.h)

# source files
set(SRCS BenchmarkClient.cpp)

add_library(client ${SRCS} ${HEADERS})
add_library(fbksd::client ALIAS client)
target_link_libraries(client PUBLIC core)
set_target_properties(client PROPERTIES OUTPUT_NAME "fbksd-client")
install(TARGETS client
    EXPORT fbksdTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY ${HEADERS_PREFIX}
    DESTINATION include/fbksd
)
