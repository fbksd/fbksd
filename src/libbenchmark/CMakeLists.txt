
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
            RenderClient.h
            exr_utils.h
            tcp_utils.h)

# source files
set(SRCS BenchmarkManager.cpp
         BenchmarkServer.cpp
         CfgParser.cpp
         RenderClient.cpp
         exr_utils.cpp
         tcp_utils.cpp)

add_library(libbenchmark ${SRCS} ${HEADERS})
add_library(fbksd::libbenchmark ALIAS libbenchmark)
target_link_libraries(libbenchmark PUBLIC core Qt5::Core ${OPENEXR_LIBRARIES} Boost::system)
target_include_directories(libbenchmark
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
if(FBKSD_MANUAL_RENDERER)
    target_compile_definitions(libbenchmark PRIVATE -DMANUAL_RENDERER)
endif()
if(FBKSD_MANUAL_CLIENT)
    target_compile_definitions(libbenchmark PRIVATE -DMANUAL_ASR)
endif()

install(TARGETS libbenchmark
    EXPORT fbksdTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
