
# Qt
set(CMAKE_AUTOMOC ON)
find_package(Qt5Core)

# OpenEXR
include_directories(/usr/include/OpenEXR)
set(OPENEXR_LIBRARIES IlmImf Half)

find_package(Boost REQUIRED system)

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
target_link_libraries(fbksd-benchmark PRIVATE core Qt5::Core ${OPENEXR_LIBRARIES} Boost::system)
if(FBKSD_MANUAL_RENDERER)
    target_compile_definitions(fbksd-benchmark PRIVATE -DMANUAL_RENDERER)
endif()
if(FBKSD_MANUAL_CLIENT)
    target_compile_definitions(fbksd-benchmark PRIVATE -DMANUAL_ASR)
endif()

install(TARGETS fbksd-benchmark
    RUNTIME DESTINATION bin
)
