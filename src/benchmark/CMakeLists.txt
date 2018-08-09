
# Qt
set(CMAKE_AUTOMOC ON)
find_package(Qt5Xml)

# OpenEXR
include_directories(/usr/include/OpenEXR)
set(OPENEXR_LIBRARIES IlmImf Half)

# header files
set(HEADERS BenchmarkManager.h
            BenchmarkServer.h
            CfgParser.h
            RenderClient.h)

# source files
set(SRCS BenchmarkManager.cpp
         BenchmarkServer.cpp
         CfgParser.cpp
         RenderClient.cpp
         main.cpp)

add_executable(fbksd-benchmark ${SRCS} ${HEADERS})
target_link_libraries(fbksd-benchmark PRIVATE core ${OPENEXR_LIBRARIES} Qt5::Xml)
install(TARGETS fbksd-benchmark
    RUNTIME DESTINATION bin
)
