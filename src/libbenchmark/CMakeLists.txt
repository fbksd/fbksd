
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
            exr_utils.h)

# source files
set(SRCS BenchmarkManager.cpp
         BenchmarkServer.cpp
         CfgParser.cpp
         RenderClient.cpp
         exr_utils.cpp)

add_library(libbenchmark ${SRCS} ${HEADERS})
target_link_libraries(libbenchmark PUBLIC core Qt5::Core ${OPENEXR_LIBRARIES} Boost::system)
target_include_directories(libbenchmark PUBLIC ${CMAKE_CURRENT_LIST_DIR})
if(FBKSD_MANUAL_RENDERER)
    target_compile_definitions(libbenchmark PRIVATE -DMANUAL_RENDERER)
endif()
if(FBKSD_MANUAL_CLIENT)
    target_compile_definitions(libbenchmark PRIVATE -DMANUAL_ASR)
endif()

add_library(fbksd::libbenchmark ALIAS libbenchmark)
