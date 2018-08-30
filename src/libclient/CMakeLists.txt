
find_package(Boost REQUIRED program_options)

set(HEADERS_PREFIX ${PROJECT_SOURCE_DIR}/include/fbksd/client)

# header files
set(HEADERS ${HEADERS_PREFIX}/BenchmarkClient.h)

# source files
set(SRCS BenchmarkClient.cpp)

add_library(client SHARED ${SRCS} ${HEADERS})
add_library(fbksd::client ALIAS client)
target_link_libraries(client PUBLIC core PRIVATE fbksd::libbenchmark Boost::program_options)
set_target_properties(client PROPERTIES
    OUTPUT_NAME "fbksd-client"
    VERSION ${PROJECT_VERSION_MAJOR}
)
target_include_directories(client PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
install(TARGETS client
    EXPORT fbksdTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY ${HEADERS_PREFIX}
    DESTINATION include/fbksd
)
