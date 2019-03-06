
find_package(Qt5Core REQUIRED)
find_package(PNG REQUIRED)
find_package(Boost REQUIRED program_options filesystem)
set(OPENEXR_LIBRARIES IlmImf Half)

set(HEADERS_PREFIX ${PROJECT_SOURCE_DIR}/include/fbksd/iqa)

add_library(iqa
    ${HEADERS_PREFIX}/iqa.h
    ${HEADERS_PREFIX}/Img.h
    iqa.cpp
    Img.cpp
)
add_library(fbksd::iqa ALIAS iqa)
target_link_libraries(iqa PRIVATE
    PNG::PNG
    ${OPENEXR_LIBRARIES}
    Boost::program_options
    Boost::filesystem
    Qt5::Core
)
set_target_properties(iqa PROPERTIES
    OUTPUT_NAME "fbksd-iqa"
    VERSION ${PROJECT_VERSION_MAJOR}
)
target_include_directories(iqa PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS iqa
    EXPORT fbksdTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY ${HEADERS_PREFIX}
    DESTINATION include/fbksd
    FILES_MATCHING PATTERN "*.h"
)
